#ifdef CHAIN_WITH_GRAPHVIZ

#include "cgraph.h"
#include "gvc.h"
#include "chain_graphviz.h"
#include "gvplugin.h"

#define ORANGE_COLOR "orange"
#define BLUE_COLOR "cornflowerblue"

Agraph_t* GvCreateGraph(void)
{
    Agraph_t* g = agopen("G", Agstrictundirected, NULL);
    agsafeset(g, "label", "Teleport chain", "");
    return g;
}

void GvDestroyGraph(Agraph_t* g)
{
    agclose(g);
}

void GvWriteGraph(Agraph_t* g)
{
    __declspec(dllimport) gvplugin_library_t gvplugin_dot_layout_LTX_library;
    __declspec(dllimport) gvplugin_library_t gvplugin_core_LTX_library;

    GVC_t* gvc = gvContext();
    gvAddLibrary(gvc, &gvplugin_dot_layout_LTX_library);
    gvAddLibrary(gvc, &gvplugin_core_LTX_library);
    gvLayout(gvc, g, "dot");
    gvRenderFilename(gvc, g, "svg", "chain.svg");
    gvFreeLayout(gvc, g);
    gvFreeContext(gvc);
}

Agnode_t* GvCreateRootNode(Agraph_t* g, bool blue)
{
    Agnode_t* n = agnode(g, "root", 1);
    agsafeset(n, "label", "P", "");
    agsafeset(n, "color", blue ? BLUE_COLOR : ORANGE_COLOR, "");
    agsafeset(n, "shape", "circle", "");
    return n;
}

#endif
