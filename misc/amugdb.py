import gdb
pp = gdb.printing.RegexpCollectionPrettyPrinter("amu")

class print_lnode_chain(gdb.Command):
    def __init__(self):
        super(print_lnode_chain, self).__init__("plnode", gdb.COMMAND_USER, gdb.COMPLETE_EXPRESSION)

    def invoke(self, arg, tty):
        try:
            val = gdb.parse_and_eval(arg)
            if val is None:
                print(f"invalid expression")
                return
            out = f"{val} -> "
            iter = val['next'].dereference()
            while iter.address != val:
                out += f"{iter.address} -> "
                iter = iter['next'].dereference()
            gdb.write(out)
        except Exception as e:
            print(f"{self.__class__.__name__} error: ", e)
print_lnode_chain()

class String_printer:
    def __init__(self, val):
        self.val = val
    
    def to_string(self):
        try:
            val = self.val
            return f"{val['s']}"
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("String", r"^amu::String$", String_printer)

class DString_printer:
    def __init__(self, val):
        self.val = val
    
    def to_string(self):
        try:
            val = self.val
            return f"{val['s']}"
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("DString", r"^amu::DString$", String_printer)

class Array_printer:
    def __init__(self, val): 
        self.val = val
    
    def display_hint(self):
        return 'array'

    def to_string(self):
        try:
            val = self.val
            if not val['count']:
                return "{empty}"
            type = str(val.type)
            subtype = type[type.find("<")+1:type.rfind(">")]
            return gdb.parse_and_eval(f"*(({subtype}*){val['data']})@{val['count']}")
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("Array", r"^amu::Array<.*>$", Array_printer)

class SharedArray_printer:
    def __init__(self, val): 
        self.val = val
    
    def display_hint(self):
        return 'array'

    def to_string(self):
        try:
            val = self.val
            if not val['count']:
                return "{empty}"
            type = str(val.type)
            subtype = type[type.find("<")+1:type.rfind(">")]
            return gdb.parse_and_eval(f"*(({subtype}*){val['data']})@{val['count']}")
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("SharedArray", r"^amu::SharedArray<.*>$", SharedArray_printer)

class MessagePart_printer:
    def __init__(self, val): 
        self.val = val

    def to_string(self):
        try:
            val:gdb.Value = self.val
            match str(val['type']):
                case "amu::MessagePart::Plain": 
                    return f"plain: {val['plain']}"
                case "amu::MessagePart::Path": 
                    return f"path: {val['plain']}"
                case "amu::MessagePart::Token": 
                    return f"token: {val['token']}"
                case "amu::MessagePart::Source": print("source")
                case "amu::MessagePart::Entity": print("entity")
                case "amu::MessagePart::Label": print("label")
                case "amu::MessagePart::Code": print("code")
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("MessagePart", r"^amu::MessagePart$", MessagePart_printer)

class Token_printer:
    def __init__(self, val):
        self.val = val
    
    def to_string(self):
        try:
            val:gdb.Value = self.val
            return f"{str(val['type'])[5:]} {val['raw']}"
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("Token", r"^amu::Token$", Token_printer)

class Label_printer:
    def __init__(self, val): self.val = val

    def to_string(self):
        try:
            val:gdb.Value = self.val
            token = val['token'].dereference()
            if val['entity']:
                entity = val['entity'].dereference()
                return f"Label {token['raw']} > {entity}"
            else:
                return f"Label {token['raw']} > null"
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("Label", r"^amu::Label$", Label_printer)

class Entity_printer: 
    def __init__(self, val): self.val = val

    def to_string(self):
        try:
            val:gdb.Value = self.val
            match str(val['type']):
                case "amu::entity::module":
                    return "module"
                case _:
                    print(f"unhandled entity type {str(val['type'])}")
                    return "err"
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("Entity", r"^amu::Entity$", Entity_printer)

class Statement_printer:
    def __init__(self, val): self.val = val

    def to_string(self):
        try:
            val:gdb.Value = self.val
            type = str(val['type'])
            match type:
                case "amu::statement::label":
                    return "label statement"
                case "amu::statement::assignment":
                    return "assignment statement"
                case "amu::statement::defer_":
                    return "defer statement"
                case "amu::statement::expression":
                    return "expression statement"
                case _:
                    print(f"unmatched statement type: {type}")
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("Statement", r"^amu::Statement$", Statement_printer)

class Pool_printer:
    def __init__(self,val): self.val = val
    def to_string(self):
        try:
            val:gdb.Value = self.val
            out = ""
            out += f"n per chunk: {val['items_per_chunk']}\n"
            out += f"chunks: {val['chunk_root']}\n"
            out += f"free blocks: {val['free_blocks']}\n"
            out += f"items: {val['items']}"
            return out
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("Pool", r"^amu::Pool<.*>$", Pool_printer)

class Tuple_printer:
    def __init__(self,val): self.val = val
    def to_string(self):
        try:
            val:gdb.Value = self.val
            type = str(val['type'])
            match type:
                case "amu::tuple::label_group":
                    return "label group tuple"
                case _:
                    gdb.write(f"unhandled tuple type: {type}")            
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("Tuple", r"^amu::Tuple$", Tuple_printer)

gdb.printing.register_pretty_printer(gdb.current_objfile(), pp)






# commands




import graphviz
import traceback

class graph_ast(gdb.Command):
    def __init__(self):
        super(graph_ast, self).__init__("gast", gdb.COMMAND_USER, gdb.COMPLETE_EXPRESSION)
    
    def build_tree(self, node:gdb.Value):
        print(f"building {node}")
        match str(node['type']):
            case "amu::node::type::label":
                self.dot.node(str(node), str(node.cast(gdb.lookup_symbol("amu::Label")[0].type.pointer()).dereference()))
            case "amu::node::type::entity":
                self.dot.node(str(node), str(node.cast(gdb.lookup_symbol("amu::Entity")[0].type.pointer()).dereference()))
            case "amu::node::type::statement":
                self.dot.node(str(node), str(node.cast(gdb.lookup_symbol("amu::Statement")[0].type.pointer()).dereference()))
            case "amu::node::type::expression":
                self.dot.node(str(node), str(node.cast(gdb.lookup_symbol("amu::Expression")[0].type.pointer()).dereference()))
            case "amu::node::type::tuple":
                self.dot.node(str(node), str(node.cast(gdb.lookup_symbol("amu::Tuple")[0].type.pointer()).dereference()))
            case _:
                print(f"unmatched type: {str(node['type'])}")
        
        current = node['first_child']
        while int(current):
            self.build_tree(current)
            self.dot.edge(str(node), str(current))
            current = current['next']

    def invoke(self, arg, tty):
        try:
            self.dot = graphviz.Digraph()
            val = gdb.parse_and_eval(arg)
            if val == None:
                print("err")
                return
            if str(val.type) != "amu::TNode" and str(val.type) != "amu::TNode *":
                print(f"gast requires a TNode or TNode* as its argument")
                return
            
            self.build_tree(val)
            self.dot.render('temp/ast')
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
graph_ast()


        
    
