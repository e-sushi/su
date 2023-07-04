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
            out = f"{val.address} -> "
            iter = val['next'].dereference()
            while iter.address != val.address:
                out += f"{iter.address} -> "
                iter = iter['next'].dereference()
            print(out)
        except Exception as e:
            print(f"error: ", e)
print_lnode_chain()

class String_printer:
    def __init__(self, val):
        self.val = val
    
    def to_string(self):
        try:
            val = self.val
            return f"{val['s']}"
        except Exception as e:
            print(f"error: {e}")
pp.add_printer("String", r"^amu::String$", String_printer)

class DString_printer:
    def __init__(self, val):
        self.val = val
    
    def to_string(self):
        try:
            val = self.val
            return f"{val['s']}"
        except Exception as e:
            print(f"error: {e}")
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
            print(f"error: {e}")
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
            print(f"error: {e}")
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
            print(f"error: {e}")
pp.add_printer("MessagePart", r"^amu::MessagePart$", MessagePart_printer)

class Token_printer:
    def __init__(self, val):
        self.val = val
    
    def to_string(self):
        try:
            val:gdb.Value = self.val
            return f"{str(val['type'])[5:]} {val['raw']}"
        except Exception as e:
            print(f"error: {e}")
pp.add_printer("Token", r"^amu::Token$", Token_printer)


gdb.printing.register_pretty_printer(gdb.current_objfile(), pp)


        
    
