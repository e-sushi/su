namespace amu {

b32 Type::
can_cast_to(Type* to)  { 
	if(this == to) return true;

	// the Whatever type is a wildcard that anything can cast to
	// but you cannot use something of this type, and checks for that
	// are handled elsewhere
	if(this->is<Whatever>() || to->is<Whatever>()) 
		return true;
	

	// all scalar types may coerce to each other
	// TODO(sushi) this should probably not allow float <-> int coercion in implicit cases, though
	if(to->is<Scalar>() && this->is<Scalar>())
		return true;

	// pointers may coerce freely
	// TODO(sushi) stronger rules may be safer, though
	// TODO(sushi) this is NOT safe
	if(to->is<Pointer>() && this->is<Pointer>())
		return true;

	// arrays can coerce between each other as long as a conversion exists
	// between their underlying types and the array being casted to is
	// smaller than the other
	if(to->is<StaticArray>() && this->is<StaticArray>()) {
		auto ato = (StaticArray*)to;
		auto athis = (StaticArray*)this;
		if(ato->count < athis->count) return false;
		if(ato->type->can_cast_to(athis->type)) return true;
	}

	// allow implicit coercion of an array to its data pointer
	// this may not be a good idea either
	if(to->is<Pointer>() && this->is<StaticArray>()) {
		auto pto = to->as<Pointer>();
		auto pthis = this->as<StaticArray>();
		return pto->type == pthis->type;
	}

	// TupleTypes can usually cast to Structured types, but whether or not 
	// the given TupleType can cast to it is determined later during the 
	// actual cast
	if(to->is<Structured>() && this->is<TupleType>()) {
		return true;
	}

	return false;
}

DString* Scalar::
display() {
	switch(this->kind) {
		case scalar::unsigned8:  return DString::create("u8");
		case scalar::unsigned16: return DString::create("u16");
		case scalar::unsigned32: return DString::create("u32");
		case scalar::unsigned64: return DString::create("u64");
		case scalar::signed8:	 return DString::create("s8");
		case scalar::signed16:	 return DString::create("s16");
		case scalar::signed32:	 return DString::create("s32");
		case scalar::signed64:	 return DString::create("s64");
		case scalar::float32:	 return DString::create("f32");
		case scalar::float64:	 return DString::create("f64");
	}
}

u64 Scalar::
size() {
	switch(this->kind) {
		case scalar::signed8:
		case scalar::unsigned8:  return 1;
		case scalar::signed16:
		case scalar::unsigned16: return 2;
		case scalar::float32:
		case scalar::signed32:
		case scalar::unsigned32: return 4;
		case scalar::float64:
		case scalar::signed64:
		case scalar::unsigned64: return 8;
	}
}

DString* Scalar::
dump() {
	DString* out = DString::create("ScalarType<");
	switch(this->kind) {
		case scalar::unsigned8:  out->append("u8");    break;
		case scalar::unsigned16: out->append("u16");   break;
		case scalar::unsigned32: out->append("u32");   break;
		case scalar::unsigned64: out->append("u64");   break;
		case scalar::signed8:	 out->append("s8");    break;
		case scalar::signed16:	 out->append("s16");   break;
		case scalar::signed32:	 out->append("s32");   break;
		case scalar::signed64:	 out->append("s64");   break;
		case scalar::float32:	 out->append("f32");   break;
		case scalar::float64:	 out->append("f64");   break;
	}
	out->append(">");
	return out;
}

DString* Scalar::
print_from_address(u8* addr) {
	DString* out = DString::create();
	switch(this->kind) {
		case scalar::unsigned8:  out->append(*(u8*)addr); break;
		case scalar::unsigned16: out->append(*(u16*)addr); break;
		case scalar::unsigned32: out->append(*(u32*)addr); break;
		case scalar::unsigned64: out->append(*(u64*)addr); break;
		case scalar::signed8:	 out->append(*(s8*)addr); break;
		case scalar::signed16:	 out->append(*(s16*)addr); break;
		case scalar::signed32:	 out->append(*(s32*)addr); break;
		case scalar::signed64:	 out->append(*(s64*)addr); break;
		case scalar::float32:	 out->append(*(f32*)addr); break;
		case scalar::float64:	 out->append(*(f64*)addr); break;
	}
	return out;
}

b32 Scalar::
is_signed() {
	return kind == scalar::signed8 || kind == scalar::signed16 || kind == scalar::signed32 || kind == scalar::signed64;
}

b32 Scalar::
is_float() {
	return kind == scalar::float32 || kind == scalar::float64;
}

b32 Scalar::
cast_to(Type* t, Expr*& n) {
	if(n->is<ScalarLiteral>()) {
		// we can just cast in place
		n->as<ScalarLiteral>()->cast_to(t);
	} else switch(t->kind) {
		case type::kind::scalar: {
			auto cast = Expr::create(expr::cast);
			cast->type = t;
			cast->start = n->start;
			cast->end = n->end;
			node::insert_above(n, cast);
			n = cast;
		} break;
		default: {
			TODO("throw diagnostic unable to cast");
		} break;
	}	
	return true;
}

Array<Pointer*> Pointer::set = Array<Pointer*>::create();

Pointer* Pointer::
create(Type* type) {
	auto [idx,found] = amu::array::util::
		search<Pointer*, Type*>(amu::Pointer::set, type, [](Pointer* p){ return p->type; });
	if(found) return amu::Pointer::set.read(idx);
	Pointer* nu = compiler::instance.storage.pointer_types.add();
	nu->type = type;
	amu::Pointer::set.insert(idx, nu);
	return nu;
}

DString* Pointer::
display() { // !Leak
	return DString::create(ScopedDeref(type->display()).x, "*");
}

DString* Pointer::
dump() {
	return display();
}

u64 Pointer::
size() {
	return sizeof(void*);
}

b32 Pointer::
cast_to(Type* t, Expr*& e) {
	TODO("handle whatever happens here");
	return 0;
}

DString* Pointer::
print_from_address(u8* addr) {
	return DString::create("(", type->display(), "*)", (u8*)*(u64*)addr);
}

Array<StaticArray*> StaticArray::set = Array<StaticArray*>::create();


// NOTE(sushi) I am VERY sorry to whoever reads or needs to fix the following functions 
//			   I am not interested in trying to setup a concrete implementation of storing 
//			   and accessing unique types yet, so the following code is stupidly scuffed
StaticArray* StaticArray::
create(Type* type, u64 count) {
	u64 hash = (u64(type) << count) * 1234;
	auto [idx, found] = amu::array::util:: // this suuuuuuuucks
		search<StaticArray*, u64>(StaticArray::set, hash, 
			[](StaticArray* a){ return (u64(a->type) << a->count) * 1234; });

	if(found) return amu::StaticArray::set.read(idx);
	StaticArray* nu = compiler::instance.storage.static_array_types.add();
	nu->type = type;
	nu->count = count;
	amu::StaticArray::set.insert(idx, nu);

	auto s = Structure::create();
	
	auto data = s->add_member("data");
	data->type = Pointer::create(type);
	data->inherited = false;
	data->offset = 0;

	auto count_ = s->add_member("count");
	count_->type = &scalar::_u64;
	count_->inherited = false;
	count_->offset = data->type->size();

	nu->structure = s;

	return nu;
}

DString* StaticArray::
display() {
	return DString::create(ScopedDeref(this->type->display()).x, "[", this->count, "]"); 
}

DString* StaticArray::
dump() {
	return display();
}

u64 StaticArray::
size() {
	return type->size() * count;
}

b32 StaticArray::
cast_to(Type* t, Expr*& e) {
	if(e->is<ArrayLiteral>()) {
		e->as<ArrayLiteral>()->cast_to(t);
	} else {
		TODO("unhandled StaticArray cast to something");
	}
	return true;
}

DString* StaticArray::
print_from_address(u8* addr) {
	auto out = DString::create();
	out->append("[");
	forI(count) {
		out->append(ScopedDeref(type->print_from_address(addr + i * type->size())).x, (i==count-1?"":","));
	}
	out->append("]");
	return out;
}

Array<ViewArray*> ViewArray::set = Array<ViewArray*>::create();

ViewArray* ViewArray::
create(Type* type) {
	u64 hash = (u64)type;
	auto [idx, found] = amu::array::util::
		search<ViewArray*, u64>(ViewArray::set, hash, [](ViewArray* a){return u64(a->type);});

	if(found) return amu::ViewArray::set.read(idx);

	ViewArray* nu = compiler::instance.storage.view_array_types.add();
	nu->type = type;
	amu::ViewArray::set.insert(idx, nu);

	auto s = Structure::create();

	auto data = s->add_member("data"); 
	data->type = Pointer::create(type);
	data->inherited = false;
	data->offset = 0;

	auto count = s->add_member("count");
	count->type = &scalar::_u64;
	count->inherited = false;
	count->offset = data->type->size();

	nu->structure = s;

	return nu;
}

DString* ViewArray::
display() {
	return DString::create(ScopedDeref(this->type->display()).x, "[]");
}

DString* ViewArray::
dump() {
	return display();
}

u64 ViewArray::
size() {
	return sizeof(void*) + sizeof(u64);
}

b32 ViewArray::
cast_to(Type* t, Expr*& e) {
	TODO("handle ViewArrays casting to something");
	return 0;
}

DString* ViewArray::
print_from_address(u8* addr) {
	TODO("print view array from address");
	return 0;
}

Array<DynamicArray*> DynamicArray::set = Array<DynamicArray*>::create();

DynamicArray* DynamicArray::
create(Type* type) {
	u64 hash = (u64)type;
	auto [idx, found] = amu::array::util::
		search<DynamicArray*, u64>(DynamicArray::set, hash, [](DynamicArray* a){return u64(a->type);});

	if(found) return DynamicArray::set.read(idx);

	DynamicArray* nu = compiler::instance.storage.dynamic_array_types.add();
	nu->type = type;
	DynamicArray::set.insert(idx, nu);

	auto s = Structure::create();

	auto data = s->add_member("data");
	data->type = Pointer::create(type);
	data->inherited = false;
	data->offset = 0;

	auto count = s->add_member("count");
	count->type = &scalar::_u64;
	count->inherited = false;
	count->offset = data->type->size();
	
	auto space = s->add_member("space");
	space->type = &scalar::_u64;
	space->inherited = false;
	space->offset = data->type->size() + count->type->size();

	nu->structure = s;

	return nu;
}

DString* DynamicArray::
display() {
	return DString::create(ScopedDeref(this->type->display()).x, "[..]");
}

DString* DynamicArray::
dump() {
	return display();
}

u64 DynamicArray::
size() { // TODO(sushi) size of Allocators when they are implemented
	return sizeof(void*) + sizeof(u64) + sizeof(u64);
}

b32 DynamicArray::
cast_to(Type* t, Expr*& e) {
	TODO("handle DynamicArrays casting to something");
	return 0;
}

DString* DynamicArray::
print_from_address(u8* addr) {
	TODO("print dynamic array from address");
	return 0;
}

Array<Range*> Range::set = Array<Range*>::create();

Range* Range::
create(Type* type) {
	u64 hash = (u64)type;
	auto [idx, found] = amu::array::util::
		search<Range*, u64>(set, hash, [](Range* a){return u64(a->type);});

	if(found) return set.read(idx);

	auto out = compiler::instance.storage.range_types.add();
	out->type = type;
	set.insert(idx, out);

	return out;
}

DString* Range::
display() {
	return DString::create("Range(TODO)");
}

DString* Range::
dump() {
	return DString::create("Range<TODO>");
}

u64 Range::
size() {
	return type->size();
}

b32 Range::
cast_to(Type* t, Expr*& e) {
	TODO("handle Ranges casting to something");
	return 0;
}

DString* Range::
print_from_address(u8* addr) {
	return type->print_from_address(addr);
}

// FunctionType does not try to be unique for now
FunctionType* FunctionType::
create() {
	FunctionType* out = compiler::instance.storage.function_types.add();
	out->kind = type::kind::function;
	out->ASTNode::kind = ast::entity;
	return out;
}

DString* FunctionType::
display() {
	DString* out = DString::create("("); 
	for(ASTNode* n = this->parameters->first_child(); n; n = n->next()) {
		out->append(n->resolve_type(), (n->next()? ", " : ""));
	}
	out->append(") -> ");
	for(ASTNode* n = this->returns; n; n = n->next()) {
		b32 block_next = n->next_is<Block>();
		out->append(n->resolve_type(), (block_next? "" : ", "));
		if(block_next) break;
	}
	return out;
}

DString* FunctionType::
dump() {
	return display();
}

u64 FunctionType::
size() {
	return sizeof(void*); // treated as pointers for now 
}

b32 FunctionType::
cast_to(Type* t, Expr*& e) {
	TODO("handle FunctionTypes casting to something");
	return 0;
}

DString* FunctionType::
print_from_address(u8* addr) {
	return DString::create((u8*)*(u64*)addr);
}

Array<TupleType*> TupleType::set = Array<TupleType*>::create();

TupleType* TupleType::
create(Tuple* tuple) {

	TupleType* nu = compiler::instance.storage.tuple_types.add();
	nu->named_elements = map::init<String, u64>();
	nu->elements = Array<Element>::create();

	b32 found_label = 0;
	b32 sizeacc = 0;
	for(ASTNode* n = tuple->first_child(); n; n = n->next()) {
		Type* t = 0;
		if(n->is<Label>()) {
			found_label = 1;
			t = n->last_child<Expr>()->type;
			u32 index = nu->elements.count;
			map::add(nu->named_elements, n->start->raw, (u64)nu->elements.count);
		} else if(found_label) {
			Assert(0); // all elements after a label should also be labels
		} else {
			t = n->as<Expr>()->type;
		}

		auto elem = nu->elements.push();
		elem->type = t;
		elem->offset = sizeacc;
		sizeacc += t->size();
	}
	
	nu->bytes = sizeacc;

	return nu;
}

DString* TupleType::
display() { // TODO(sushi) this sucks
	auto out = DString::create("(");
	forI(elements.count-1) {
		out->append(ScopedDeref(elements.read(i).type->display()).x, ", ");
	}

	out->append(ScopedDeref(elements.read(-1).type->display()).x, ")");
	return out;
}

DString* TupleType::
dump() {
	return DString::create("TupleType<", ScopedDeref(display()).x, ">");
}

u64 TupleType::
size() {
	return bytes;

}

namespace type::internal {

// TODO(sushi) move this back to Sema somehow 
b32
structure_initializer(Structured* st, TupleLiteral* from) {
	auto t = from->first_child<Tuple>();
	auto tt = t->type->as<TupleType>();

	// make sure the structure we're initializing has 
	// been through sema first
	// TODO(sushi) from->code is probably gonna be null here
	if(!st->code->wait_until_level(code::sema, from->code)) return false;

	// TODO(sushi) this can probably be done better
	//			   but this is the cleanest solution I can
	//			   currently think of.

	// the method here is to reorganize the TupleLiteral
	// so that its elements are correctly positioned and 
	// sized to be directly copied to the thing we are initializing

	auto ordered = Array<Expr*>::create();
	ordered.resize(st->structure->members.keys.count);
	auto to_be_filled = Array<Member*>::create();
	
	for(auto m = st->structure->first_member; m; m = m->next<Member>()) {
		to_be_filled.push(m);
	}

	auto titer = t->first_child();
	auto miter = st->structure->first_member;
	while(titer && titer->is_not<Label>()) {
		if(!miter) {
			diagnostic::sema::
				tuple_struct_initializer_too_many_elements(titer->start);
			return false;
		}
		auto e = titer->as<Expr>();
		if(!e->type->can_cast_to(miter->type)) {
			diagnostic::sema::
				tuple_struct_initializer_cannot_cast_expr_to_member(e->start, e->type, miter->start->raw, miter->type);
			return false;
		}

		if(!e->type->cast_to(miter->type, e)) return false;

		to_be_filled.readref(miter->index) = 0;
		ordered.readref(miter->index) = e;	
		
		titer = e->next();
		miter = miter->next<Member>();
	}

	// at this point, any remaining tuple elements are named
	// so we just search for them
	while(titer) {
		if(!miter) {
			diagnostic::sema::
				tuple_struct_initializer_too_many_elements(titer->start);
			return false;
		}
		auto m = st->find_member(titer->start->raw);
		if(!m) {
			diagnostic::sema::
				tuple_struct_initializer_unknown_member(titer->start, titer->start->raw, st);
			return false;
		}

		if(!to_be_filled.read(m->index)) {
			// TODO(sushi) show which element already satisfied the member
			diagnostic::sema::
				tuple_struct_initializer_named_member_already_satisfied(titer->start, m->start->raw);
			return false;
		}

		auto e = titer->last_child<Expr>();

		if(e->type != m->type) {
			if(!e->type->can_cast_to(m->type)) {
				diagnostic::sema::
					tuple_struct_initializer_cannot_cast_expr_to_member(e->start, e->type, miter->start->raw, miter->type);
				return false;
			}

			if(!e->type->cast_to(m->type, e)) return false;
		}

		ordered.readref(m->index) = e;
		to_be_filled.readref(m->index) = 0;
		titer = titer->next();
	}

	forI(to_be_filled.count) {
		auto m = to_be_filled.read(i);
		if(m) {
			// TODO(sushi) handle zero filling/initializing other types
			auto e = ScalarLiteral::create()->as<Expr>();
			e->as<ScalarLiteral>()->cast_to(&scalar::_u64);
			e->as<ScalarLiteral>()->value._u64 = 0;
			if(!e->type->cast_to(m->type, e)) return false;
			ordered.readref(i) = e;
		}
	}

	while(t->first_child()) {
		node::change_parent(0, t->first_child());
	}

	forI(ordered.count) {
		node::insert_last(t, ordered.read(i));
	}

	t->type = TupleType::create(t);

	return true;
}

} // namespace type::internal

b32 TupleType::
cast_to(Type* t, Expr*& e) {
	if(t->is<Structured>() && e->is<TupleLiteral>()) {
		return type::internal::structure_initializer(t->as<Structured>(), e->as<TupleLiteral>());
	} else if(t->is<Structured>() && e->type->is<TupleType>()) {
		diagnostic::sema::
			tuple_cast_to_structured_not_yet_supported(e->start);
		return false;
	} else if(t->is<TupleType>()){
		TODO("handle tuple to tuple casting");
	} else {
		TODO("unhandled TupleType cast case");
	}
	return 0;
}

DString* TupleType::
print_from_address(u8* addr) {
	TODO("print tuples from address");
	return 0;
}

// namespace type::structure {
// Array<ExistingStructureType> set = amu::Array<ExistingStructureType>::create();
// } // namespace structure

// Structured* Structured::
// create(Structure* s) {
//	   auto [idx, found] = amu::type::structure::set.util::
//		   search<type::structure::ExistingStructureType, Structure*>(s, [](type::structure::ExistingStructureType& s) { return s.structure; });
//	   if(found) return amu::type::structure::set.read(idx).stype;
//	   type::structure::ExistingStructureType* nu = amu::structure::set.insert(idx);
//	   nu->stype = compiler::instance.storage.structured_types.add();
//	   nu->stype->kind = type::kind::structured;
//	   nu->stype->node.kind = node::type;
//	   nu->stype->structure = s;
//	   nu->structure = s;
//	   return nu->stype;
// }

Structured* Structured::
create(Structure* s) {
	auto out = compiler::instance.storage.structured_types.add();
	out->structure = s;
	return out;
}

Member* Structured::
find_member(String id) {
	return structure->find_member(id);
}

DString* Structured::
display() {
	return label->display();
}

DString* Structured::
dump() {
	return DString::create("Structured<TODO>");
}

u64 Structured::
size() {
	return structure->size;
}

b32 Structured::
cast_to(Type* t, Expr*& e) {
	TODO("handled Structured types casting to something");
	return 0;
}

DString* Structured::
print_from_address(u8* addr) {
	return structure->display_members_from_address(addr);
}

} // namespace amu

