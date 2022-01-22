#include "graphviz/gvc.h"

Agraph_t* gvgraph = 0;
GVC_t* gvc = 0;
u32 colidx = 1;
u32 groupid = 0;

void make_dot_file(Node* node, Agnode_t* parent, Node* align_to) {
	static u32 i = 0;
	i++;
	u32 save = i;
	u32 colsave = colidx;
	
	Agnode_t* me = agnode(gvgraph, to_string(i).str, 1);
	agset(me, "label", node->comment.str);
	agset(me, "color", to_string(colsave).str);
	
	for_node(node->first_child){
		if (node->first_child->next) {
			align_to = node;
			make_dot_file(n, me, node);
		}
		else {
			make_dot_file(node->first_child, me, align_to);
		}
		colidx = (colidx + 1) % 11 + 1;
	}
	agset(me, "group", to_string(align_to).str);
	
	Agedge_t* edge = agedge(gvgraph, parent, me, "", 1);
	agset(edge, "color", to_string(colsave).str);
	
}

void generate_ast_graph_svg(const char* filepath, Program& program){
	gvc = gvContext();
	gvgraph = agopen("ast tree", Agdirected, 0);
	agattr(gvgraph, AGNODE, "fontcolor",   "white");
	agattr(gvgraph, AGNODE, "color",       "1");
	agattr(gvgraph, AGNODE, "shape",       "box");
	agattr(gvgraph, AGNODE, "margins",     "0.08");
	agattr(gvgraph, AGNODE, "group",       "0");
	agattr(gvgraph, AGNODE, "width",       "0");
	agattr(gvgraph, AGNODE, "height",      "0");
	agattr(gvgraph, AGNODE, "colorscheme", "rdylbu11");
	agattr(gvgraph, AGEDGE, "color",       "white");
	agattr(gvgraph, AGEDGE, "colorscheme", "rdylbu11");
	agattr(gvgraph, AGEDGE, "style",       "");
	agattr(gvgraph, AGEDGE, "arrowhead",   "none");
	agattr(gvgraph, AGEDGE, "penwidth",    "0.5");
	agattr(gvgraph, AGEDGE, "constraint",  "true");
	agattr(gvgraph, AGRAPH, "bgcolor",     "grey12");
	agattr(gvgraph, AGRAPH, "concentrate", "true");
	agattr(gvgraph, AGRAPH, "splines",     "true");
	
	Agnode_t* prog = agnode(gvgraph, "program", 1);
	
	for_node(program.node.first_child){
		make_dot_file(n, prog, n);
	}
	
	gvLayout(gvc, gvgraph, "dot");
	gvRenderFilename(gvc, gvgraph, "svg", filepath);
}