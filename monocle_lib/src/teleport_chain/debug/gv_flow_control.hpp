#pragma once

#include "monocle_config.hpp"
#include "teleport_chain/generate_state.hpp"

#include <vector>
#include <ostream>

namespace mon {

/*
* A class that records some of the flow control of GenerateTeleportChain(). Then it can be
* converted to a graphviz (.gv/.dot) file and viewed.
*/
class GraphvizFlowControlResult {

    // slice of all_queued_vals
    struct queue_chunk {
        size_t start, end;
    };

    enum class PlaneSide : int {
        Behind = -1,
        InFront = 1,
        Unknown = 0,
    };

    struct teleport_func {
        // index into call queued func list
        size_t call_queued_parent_index;
        // is this a blue portal that did the teleport?
        size_t blue : 1;
        PlaneSide plane_side : 2;
        // the cumulative teleport count when this was first called
        int cum_tp : 29;
    };

    struct call_queued_func {
        // index into teleport func list
        size_t teleport_parent_index : 30;
        // from the teleport func, which touch call are we?
        size_t port : 2;
        // what the queue looked like when this function was first called
        queue_chunk chunk;
    };

    // exists if params.n_max_teleports was exceeded
    struct {
        size_t call_queued_parent_index;
        bool exists = false;
        bool blue = true;
    } tp_exceeded_node;

    std::vector<TeleportChainInternalState::queue_entry> all_queued_vals;
    std::vector<teleport_func> teleports;
    std::vector<call_queued_func> call_queueds;

    struct chain_state {
        size_t cur_fn_index = 0;
        bool in_teleport_fn = false;
        bool blue_primary = false;
        size_t cur_queue_start = 0;
        int cur_cum_tp = 0;
        size_t last_teleport_port = 0;
    } state;

    friend struct GenerateTeleportChainImpl;

    void Clear();
    void QueueFunc(TeleportChainInternalState::queue_entry fn);
    void DequeueFunc();
    void TpExceeded(bool last_tp_blue);
    void PostTeleportTransform(bool blue, PlaneSide plane_side);
    void CallQueuedPostAddNull();
    void FuncLeave();

public:
    GraphvizFlowControlResult() = default;

    // write the recorded state to a .gv file
    friend std::ostream& operator<<(std::ostream& os, GraphvizFlowControlResult const& res);
};

} // namespace mon
