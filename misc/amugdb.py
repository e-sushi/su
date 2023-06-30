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
    
