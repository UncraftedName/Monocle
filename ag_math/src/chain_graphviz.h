#ifdef CHAIN_WITH_GRAPHVIZ

#include "cgraph.h"

#ifdef __cplusplus
extern "C" {
#endif

Agraph_t* GvCreateGraph(void);
void GvDestroyGraph(Agraph_t* g);
void GvWriteGraph(Agraph_t* g);

Agnode_t* GvCreateRootNode(Agraph_t* g, bool blue);
Agnode_t* GvCreateTeleportNode(Agnode_t* prev_root, Agraph_t* g, int cum_teleports, int ulp_diffs);
void GvIncrementTeleportNodePort(Agnode_t* n);

#ifdef __cplusplus
}
#endif

#endif
