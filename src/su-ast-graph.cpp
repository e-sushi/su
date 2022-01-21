#include "graphviz/gvc.h"

Agraph_t* gvgraph = 0;
GVC_t* gvc = 0;
u32 colidx = 1;

Agnode_t* make_dot_file(Node* node, Agnode_t* parent) {
	static u32 i = 0;
	i++;
	u32 save = i;
	u32 colsave = colidx;
	
	string send = node->debug_str;
	
	Agnode_t* me = agnode(gvgraph, to_string(i).str, 1);
	agset(me, "label", send.str);
	agset(me, "color", to_string(colsave).str);
	
	Agnode_t* ret = me;
	
	if (node->first_child) {
		ret = make_dot_file(node->first_child, me);
		if (node->first_child != node->last_child) {
			ret = me;
		}
	}
	if (node->parent && node->next != node->parent->first_child) {
		colidx = (colidx + 1) % 11 + 1;
		make_dot_file(node->next, parent);
	}
	
	if (parent) {
		Agedge_t* edge = agedge(gvgraph, parent, me, "", 1);
		agset(edge, "color", to_string(colsave).str);
		//if (node->next != node) { 
		//	agset(edge, "constraint", "false"); 
		//}
	}
	
	//TODO figure out how to make columns stay in line
	if (ret != me && node->next != node) {
		//Agedge_t* edge = agedge(gvgraph, me, ret, "", 1);
		//agset(edge, "weight", "10");
		//agset(edge, "style", "invis");
		//agset(edge, "constraint", "false");
		
	}
	
	return ret;
}

void generate_ast_graph_svg(const char* filepath, Program& program){
	gvc = gvContext();
	gvgraph = agopen("ast tree", Agdirected, 0);
	agattr(gvgraph, AGNODE, "fontcolor",   "white");
	agattr(gvgraph, AGNODE, "color",       "1");
	agattr(gvgraph, AGNODE, "shape",       "box");
	agattr(gvgraph, AGNODE, "margins",     "0.08");
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
	
	make_dot_file(&program.node, 0);
	gvLayout(gvc, gvgraph, "dot");
	gvRenderFilename(gvc, gvgraph, "svg", filepath);
}