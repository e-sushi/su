import gdb
pp = gdb.printing.RegexpCollectionPrettyPrinter("amu")

import time

def dbgmsg(s):
    t = time.perf_counter()
    gdb.write(f"dbg:{t}: {s}"); gdb.flush()

def full_deref_if_ptr(thing):
    while thing.type.code == gdb.TYPE_CODE_PTR:
        thing = thing.dereference()
    return thing

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
            ptr = int(val['str'])
            # this must be a corrupt String
            if abs(val['count']) > int(10000):
                return "corrupt String"
            buf = gdb.selected_inferior().read_memory(ptr, val['count']).tobytes().decode()
            buf = buf.replace('\n', '\\n')
            return buf
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("String", r"^amu::String$", String_printer)

class DString_printer:
    def __init__(self, val):
        self.val = val
    
    def to_string(self):
        try:
            val = self.val
            return f"{val['fin']}"
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
            subtype = type[type.find("<")+1:-1]
            ptr = val['data']
            out = "{\n"
            for i in range(val['count']):
                out += "  " + str(full_deref_if_ptr(gdb.parse_and_eval(f"*((({subtype}*){ptr})+{i})"))) + " ,\n"
            out += "}"
            return out
            # return gdb.parse_and_eval(f"*(({subtype}*){val['data']})@{val['count']}")
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
            match str(val['kind']):
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
            kind = str(val['kind'])[12:]
            file = str(val['source']['name']).strip('"')
            line = str(val['l0'])
            col = str(val['c0'])
            raw = str(val['raw'])
            return f"{file}:{line}:{col}:{kind} {raw}"
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("Token", r"^amu::Token$", Token_printer)

class Label_printer:
    def __init__(self, val): self.val = val

    def to_string(self):
        try:
            val:gdb.Value = self.val
            out = "label "
            start = ""
            end = ""
            if val['node']['start']:
                start = str(val['node']['start'].dereference())
            else:
                start = "<null start>"
            if val['node']['end']:
                end = str(val['node']['end'].dereference())
            else:
                end = "<null end>"
            out += f"{start} -> {end}"
            return out
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("Label", r"^amu::Label$", Label_printer)

class Statement_printer:
    def __init__(self, val): self.val = val

    def to_string(self):
        try:
            val:gdb.Value = self.val
            out = f"{str(val['kind'])[5:].replace('statement', 'stmt')} "
            start = ""
            end = ""
            if val['node']['start']:
                start = str(val['node']['start'].dereference())
            else:
                start = "<null start>"
            if val['node']['end']:
                end = str(val['node']['end'].dereference())
            else:
                end = "<null end>"
            out += f"{start} -> {end}"
            return out
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("Statement", r"^amu::Statement$", Statement_printer)

class Function_printer:
    def __init__(self, val): self.val = val

    def to_string(self):
        try:
            val:gdb.Value = self.val
            out = "function "
            start = ""
            end = ""
            if val['node']['start']:
                start = str(val['node']['start'].dereference())
            else:
                start = "<null start>"
            if val['node']['end']:
                end = str(val['node']['end'].dereference())
            else:
                end = "<null end>"
            out += f"{start} -> {end}"
            return out
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("Function", r"^amu::Function$", Function_printer)

class Module_printer:
    def __init__(self, val): self.val = val

    def to_string(self):
        try:
            val:gdb.Value = self.val
            out = "module "
            start = ""
            end = ""
            if val['node']['start']:
                start = str(val['node']['start'].dereference())
            else:
                start = "<null start>"
            if val['node']['end']:
                end = str(val['node']['end'].dereference())
            else:
                end = "<null end>"
            out += f"{start} -> {end}"
            return out
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("Module", r"^amu::Module$", Module_printer)

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
            out = f"{str(val['kind'])[5:]} "
            start = ""
            end = ""
            if val['node']['start']:
                start = str(val['node']['start'].dereference())
            else:
                start = "<null start>"
            if val['node']['end']:
                end = str(val['node']['end'].dereference())
            else:
                end = "<null end>"
            out += f"{start} -> {end}"
            return out
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("Tuple", r"^amu::Tuple$", Tuple_printer)

class TNode_printer:
    def __init__(self, val): self.val = val
    def to_string(self):
        try:
            val:gdb.Value = self.val
            kind = str(val['kind'])
            match kind:
                case "amu::node::label":
                    return gdb.parse_and_eval(f"*(amu::Label*){val.address}")
                case "amu::node::tuple":
                    return gdb.parse_and_eval(f"*(amu::Tuple*){val.address}")
                case "amu::node::expression":
                    return gdb.parse_and_eval(f"*(amu::Expression*){val.address}")
                case "amu::node::statement":
                    return gdb.parse_and_eval(f"*(amu::Statement*){val.address}")
                case "amu::node::function":
                    return gdb.parse_and_eval(f"*(amu::Function*){val.address}")
                case "amu::node::structure":
                    return gdb.parse_and_eval(f"*(amu::Structure*){val.address}")
                case "amu::node::module":
                    return gdb.parse_and_eval(f"*(amu::Module*){val.address}")
                case _:
                    return f"unhandled node kind: {kind}"
                
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("TNode", r"^amu::TNode", TNode_printer)

class Expression_printer:
    def __init__(self, val): self.val = val
    def to_string(self):
        try:
            val:gdb.Value = self.val
            out = f"{str(val['kind']).lstrip('amu::').replace('expression', 'expr')} "
            start = ""
            end = ""
            node = val['node']
            s = node['start']
            e = node['end']
            if s:
                start = str(s.dereference())
            else:
                start = "<null start>"
            if e:
                end = str(e.dereference())
            else:
                end = "<null end>"
            out += f"{start} -> {end}"
            return out
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("Expression", r"^amu::Expression", Expression_printer)

class Map_printer: 
    def __init__(self, val): self.val = val
    def to_string(self):
        try:
            val:gdb.Value = self.val

        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
# pp.add_printer("Map", r"^amu::Map<.*?,.*>$", Map_printer)

gdb.printing.register_pretty_printer(gdb.current_objfile(), pp)




# commands




import graphviz
import traceback

class graph_ast(gdb.Command):
    def __init__(self):
        super(graph_ast, self).__init__("gast", gdb.COMMAND_USER, gdb.COMPLETE_EXPRESSION)
    
    def build_tree(self, node:gdb.Value):
        print(f"building {node.dereference()}")
        label = ""
        match str(node['kind']):
            case "amu::node::label":
                label = str(node.cast(gdb.lookup_symbol("amu::Label")[0].type.pointer()).dereference())
            case "amu::node::entity":
                label = str(node.cast(gdb.lookup_symbol("amu::Entity")[0].type.pointer()).dereference())
            case "amu::node::statement":
                label = str(node.cast(gdb.lookup_symbol("amu::Statement")[0].type.pointer()).dereference())
            case "amu::node::expression":
                label = str(node.cast(gdb.lookup_symbol("amu::Expression")[0].type.pointer()).dereference())
            case "amu::node::tuple":
                label = str(node.cast(gdb.lookup_symbol("amu::Tuple")[0].type.pointer()).dereference())
            case "amu::node::module":
                label = str(node.cast(gdb.lookup_symbol("amu::Module")[0].type.pointer()).dereference())
            case "amu::node::function":
                label = str(node.cast(gdb.lookup_symbol("amu::Function")[0].type.pointer()).dereference())
            case _:
                print(f"unmatched type: {str(node['kind'])}")
        label = label.split(' ')[0]
        label += f"\n{node['start'].dereference()}\n{node['end'].dereference()}"
        self.dot.node(str(node), label)
        
        current = node['first_child']
        while int(current):
            self.build_tree(current)
            self.dot.edge(str(node), str(current))
            current = current['next']

    def invoke(self, arg, tty):
        try:
            self.dot = graphviz.Digraph()
            self.dot.attr('graph', bgcolor='black', splines='true', concentrate='true')
            self.dot.attr('edge', color='white', arrowhead="none", penwidth='0.5', constraint='true')
            self.dot.attr('node', shape='box', fontcolor='white', color='white', margins='0.08', height='0', width='0', group='0')
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
            #try to output whatever we got anyways 
            self.dot.render('temp/ast')
graph_ast()

# class graph_ast_stack(gdb.Command):
#     def __init__(self):
#         super(graph_ast_stack, self).__init__("gasts", gdb.COMMAND_USER, gdb.COMPLETE_EXPRESSION)
    
#     def invoke(self, args, tty):
#         stack = gdb.parse_and_eval("amu::parser::internal::stack")
#         if stack == None:
#             print("unable to acquire parser stack")
#             return
#         for i in range(stack['count']):
#             elem = gdb.parse_and_eval()
# graph_ast_stack()

class print_ast(gdb.Command):
    def __init__(self):
        super(print_ast, self).__init__("past", gdb.COMMAND_USER, gdb.COMPLETE_EXPRESSION)

    def build_string(self, node:gdb.Value):
        gdb.write(f"build_string {node}\n")
        gdb.flush()
        if node['first_child']:
            self.out += "("

        match str(node['kind']):
            case "amu::node::label":
                self.out += str(node.cast(gdb.lookup_symbol("amu::Label")[0].type.pointer()).dereference())
            case "amu::node::entity":
                self.out += str(node.cast(gdb.lookup_symbol("amu::Entity")[0].type.pointer()).dereference())
            case "amu::node::statement":
                self.out += str(node.cast(gdb.lookup_symbol("amu::Statement")[0].type.pointer()).dereference())
            case "amu::node::expression":
                self.out += str(node.cast(gdb.lookup_symbol("amu::Expression")[0].type.pointer()).dereference())
            case "amu::node::tuple":
                self.out += str(node.cast(gdb.lookup_symbol("amu::Tuple")[0].type.pointer()).dereference())
            case "amu::node::module":
                self.out += str(node.cast(gdb.lookup_symbol("amu::Module")[0].type.pointer()).dereference())
            case _:
                print(f"unmatched kind: {str(node['kind'])}")
        
        current = node['first_child']
        if current:
            self.layers += 1
            while int(current):
                self.out += "\n" + " " * self.layers * 2
                self.build_string(current)
                current = current['next']
            self.out += ")"
            self.layers -= 1

    def invoke(self, arg, tty):
        self.out = ""
        self.layers = 0
        try: 
            val = gdb.parse_and_eval(arg)
            if val == None:
                print("failed to parse given argument")
                return 
            if str(val.type) != "amu::TNode" and str(val.type) != "amu::TNode *":
                print(f"past requires a TNode ot TNode* as its argument")
                return
            self.build_string(val)
            gdb.write(self.out)
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
print_ast()

class parser_print_stack(gdb.Command):
    def __init__(self):
        super(parser_print_stack, self).__init__("pstack", gdb.COMMAND_USER)
    
    def invoke(self, args, tty):
        try:
            val = gdb.parse_and_eval("stack")
            out = ""
            for i in range(val['count']):
                elem = gdb.parse_and_eval(f"*(stack.data + {i})")
                to_write = None
                if not elem:
                    to_write = "stack err"
                else:
                    to_write = str(elem.dereference())
                if not tty:
                    out += to_write + '\n'
                else:
                    print(to_write)
            return out
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
parser_print_stack()

class parser_track_stack(gdb.Command):
    def __init__(self):
        super(parser_track_stack, self).__init__("tstack", gdb.COMMAND_USER)
    
    def invoke(self, args, tty):
        gdb.rbreak(r'amu::parser::internal::__stack_pop')
        gdb.rbreak(r'amu::parser::internal::__stack_push')
        gdb.execute("r")
        pps = parser_print_stack()
        out = open("temp/track_stack", "w")

        # we need to load the source's buffer manually, because gdb with attempt to 
        # fold repeating things in arrays, including strings!
        bufferptr = int(gdb.parse_and_eval("parser->source->buffer->str"))

        length = 0
        while 1:
            byte = int.from_bytes(gdb.selected_inferior().read_memory(bufferptr+length, 1)[0])
            if not byte:
                break
            length += 1

        buffer = gdb.selected_inferior().read_memory(bufferptr, length).tobytes().decode()
        lastln = 0
        lines = []
        for i,c in enumerate(buffer):
            if c == '\n':
                lines.append(buffer[lastln:i])
                lastln = i+1
        lines.append(buffer[lastln:])

        while gdb.selected_inferior().connection_num != None:
            try:
                curfunc = gdb.selected_frame().function()
                if curfunc == None:
                    gdb.execute("c")
                    continue
                if "stack_push" in curfunc.name:
                    out.write(f"push -------------------------------- {gdb.parse_and_eval('caller')} push\n")
                    gdb.execute("finish")
                if "stack_pop" in curfunc.name:
                    out.write(f"pop -------------------------------- {gdb.parse_and_eval('caller')} pop\n")

                out.write(pps.invoke(None, False))
                curt = gdb.parse_and_eval('curt')
                line = lines[int(curt['l0'])-1]
                linenum = str(int(curt['l0']))
                col = int(curt['c0'])-1 + len(linenum) + 1
                second_line = None
                if curt['l0'] != len(lines):
                    second_line = lines[curt['l0']]
                    second_linenum = str(curt['l0']+1)
                    if len(second_linenum) > len(linenum):
                        linenum = linenum.rjust(len(second_linenum))
                out.write(" "*col+"v\n")
                out.write(linenum + " " + line + '\n')
                if second_line:
                    out.write(second_linenum + " " + second_line + '\n')
                out.write("\n\n")
                gdb.execute("c")
            except Exception as e:
                print(f"{self.__class__.__name__} error: {e}")
                return 
        gdb.execute("del")
parser_track_stack()
    
class track_locks(gdb.Command):
    def __init__(self):
        super(track_locks, self).__init__("tlocks", gdb.COMMAND_USER)
    
    def invoke(self, args, tty):
        gdb.rbreak("mutex_lock")
        gdb.rbreak("mutex_unlock")
        gdb.rbreak("shared_mutex_lock")
        gdb.rbreak("shared_mutex_unlock")
        
        out = open("temp/track_lock", "w")

        gdb.execute("r")

        while gdb.selected_inferior().connection_num != None:
            try:
                addr = str(gdb.parse_and_eval('m'))
                addr = addr[addr.find("0x"):][:addr.find(" ")]
                cmd = f"info symbol {addr}"
                line = gdb.execute(cmd, to_string=True)[:-1]

                curfunc = gdb.selected_frame().function()
                if "unlock" in curfunc.name:
                    line += " unlock " + str(gdb.selected_thread().global_num) + "\n"
                else:
                    line += " lock " + str(gdb.selected_thread().global_num) + "\n"

                out.write(line)
                
                gdb.execute("c")
            except Exception as e:
                print(f"{self.__class__.__name__} error: {e}")
                return 
track_locks()

import time

class trace_parser(gdb.Command):
    def __init__(self):
        super(trace_parser, self).__init__("tparse", gdb.COMMAND_USER)
    
    def invoke(self, args, tty):
        gdb.rbreak("parser::internal::*")

        out = open("temp/trace_parser", "w")

        gdb.execute("r")

        while gdb.selected_inferior().connection_num != None:
            try:
                curtime = time.perf_counter()
                threadnum = gdb.selected_thread().global_num
                curfunc = gdb.selected_frame().function()

                out.write(f"{curtime}:  {threadnum}  :{curfunc.name}\n")
                gdb.execute("c")

            except Exception as e:
                print(f"{self.__class__.__name__} error: {e}")
                return 
trace_parser()