#ifdef CHAIN_WITH_GRAPHVIZ

#include "cgraph.h"
#include "gvc.h"
#include "chain_graphviz.h"
#include "gvplugin.h"

#define ORANGE_COLOR "orange"
#define BLUE_COLOR "cornflowerblue"

static GVC_t* g_gvc;

void GvInitGlobalContext(void)
{
    __declspec(dllimport) gvplugin_library_t gvplugin_dot_layout_LTX_library;
    __declspec(dllimport) gvplugin_library_t gvplugin_core_LTX_library;

    g_gvc = gvContext();
    gvAddLibrary(g_gvc, &gvplugin_dot_layout_LTX_library);
    gvAddLibrary(g_gvc, &gvplugin_core_LTX_library);
}

void GvDestroyGlobalContext(void)
{
    gvFreeContext(g_gvc);
}

Agraph_t* GvCreateGraph(void)
{
    Agraph_t* g = agopen("G", Agstrictundirected, NULL);
    // agsafeset(g, "label", "Teleport chain", "");
    agattr(g, AGNODE, "shape", "record");
    return g;
}

void GvDestroyGraph(Agraph_t* g)
{
    if (g)
        agclose(g);
}

void GvWriteGraph(Agraph_t* g, const char* format, const char* filename)
{
    if (!g)
        return;
    gvLayout(g_gvc, g, "dot");
    gvRenderFilename(g_gvc, g, format, filename);
    gvFreeLayout(g_gvc, g);
}

Agnode_t* GvCreateRootNode(Agraph_t* g, bool blue)
{
    if (!g)
        return NULL;
    Agnode_t* n = agnode(g, "root", 1);
    agsafeset(n, "label", "P", "");
    agsafeset(n, "color", blue ? BLUE_COLOR : ORANGE_COLOR, "");
    agsafeset(n, "shape", "circle", "");
    return n;
}

Agnode_t* GvCreateCallQueuedNode(Agraph_t* g,
                                 Agnode_t* cur_root,
                                 bool queued_teleport,
                                 const char* label,
                                 int from_touch_call_idx)
{
    if (!g)
        return NULL;
    Agnode_t* n = agnode(g, NULL, 1);
    agsafeset(n, "label", label, "");
    Agedge_t* e = agedge(g, cur_root, n, NULL, 1);
    if (!queued_teleport)
        agsafeset(e, "style", "dotted", "");
    else
        agsafeset(e, "color", "red", "");
    if (from_touch_call_idx >= 0) {
        assert(from_touch_call_idx < 4);
        char tailport[2] = {(char)('0' + from_touch_call_idx), '\0'};
        agsafeset(e, "tailport", tailport, "");
    }
    return n;
}

Agnode_t* GvCreateTeleportNode(Agraph_t* g, Agnode_t* cur_root, bool blue, int cum_teleports, int ulp_diffs)
{
    if (!g)
        return NULL;
    char label[512];
    snprintf(label,
             sizeof label,
             "<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"0\">"
             "<TR><TD colspan=\"4\">%d%s</TD></TR>"
             "<TR>"
             "<TD PORT=\"0\">E</TD>"
             "<TD PORT=\"1\" BGCOLOR=\"%s\">P</TD>"
             "<TD PORT=\"2\" BGCOLOR=\"%s\">P</TD>"
             "<TD PORT=\"3\">E</TD>"
             "</TR>"
             "</TABLE>",
             cum_teleports,
             ulp_diffs == INVALID_ULP_DIFF ? "" : (ulp_diffs < 0 ? "F" : "B"),
             blue ? ORANGE_COLOR : BLUE_COLOR,
             blue ? ORANGE_COLOR : BLUE_COLOR);
    Agnode_t* n = agnode(g, NULL, 1);
    agsafeset(n, "label", agstrdup_html(g, label), "");
    agsafeset(n, "shape", "plaintext", "");
    agsafeset(n, "color", blue ? BLUE_COLOR : ORANGE_COLOR, "");
    Agedge_t* e = agedge(g, cur_root, n, NULL, 1);
    agsafeset(e, "style", "dotted", "");
    return n;
}

#endif
