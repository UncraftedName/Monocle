#ifdef CHAIN_WITH_GRAPHVIZ

#include "limits.h"

#include "cgraph.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INVALID_ULP_DIFF INT_MAX

void GvInitGlobalContext(void);
void GvDestroyGlobalContext(void);

Agraph_t* GvCreateGraph(void);
void GvDestroyGraph(Agraph_t* g);
void GvWriteGraph(Agraph_t* g, const char* format, const char* filename);

Agnode_t* GvCreateRootNode(Agraph_t* g, bool blue);
Agnode_t* GvCreateCallQueuedNode(Agraph_t* g,
                                 Agnode_t* cur_root,
                                 bool queued_teleport,
                                 const char* label,
                                 int from_touch_call_idx);
Agnode_t* GvCreateTeleportNode(Agraph_t* g, Agnode_t* cur_root, bool blue, int cum_teleports, int ulp_diffs);
Agnode_t* GvCreateExceededTpNode(Agraph_t* g, Agnode_t* cur_root, bool blue);

#ifdef __cplusplus
}
#endif

#endif
