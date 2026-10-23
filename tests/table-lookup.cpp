// Independent sparse-table oracle: empty/dense rows, holes, defaults and bounds.
#include "internal/table/TableView/TableView.hpp"
#include <vector>
#include <iostream>
#include <cstring>
#include <stdexcept>
int main() {
    const size_t sizes[]={0,1,2,3,7,16,64,128,257};
    const size_t states=sizeof(sizes)/sizeof(sizes[0]);
    size_t entries=0;
    for(size_t s=0;s<states;++s) entries+=sizes[s];
    size_t total=sizeof(t_table_blob_hdr)+states*sizeof(t_action_span)+entries*sizeof(t_action_entry)
        +states*sizeof(t_blob_span)+entries*sizeof(t_goto_entry)+sizeof(t_rule_entry);
    std::vector<long double> storage((total+sizeof(long double)-1)/sizeof(long double));
    unsigned char* base=reinterpret_cast<unsigned char*>(&storage[0]);
    std::memset(base,0,total);
    t_table_blob_hdr* hdr=reinterpret_cast<t_table_blob_hdr*>(base);
    hdr->total_size=total;hdr->state_count=states;hdr->non_terminal_count=257;
    hdr->rule_count=1;hdr->semantic_count=0;hdr->action_count=entries;hdr->goto_count=entries;
    size_t offset=sizeof(*hdr);hdr->action_spans_offset=offset;offset+=states*sizeof(t_action_span);
    hdr->action_entries_offset=offset;offset+=entries*sizeof(t_action_entry);
    hdr->goto_spans_offset=offset;offset+=states*sizeof(t_blob_span);
    hdr->goto_entries_offset=offset;offset+=entries*sizeof(t_goto_entry);hdr->rule_entries_offset=offset;
    t_action_span* spans=reinterpret_cast<t_action_span*>(base+hdr->action_spans_offset);
    t_action_entry* actions=reinterpret_cast<t_action_entry*>(base+hdr->action_entries_offset);
    t_blob_span* gotos=reinterpret_cast<t_blob_span*>(base+hdr->goto_spans_offset);
    t_goto_entry* targets=reinterpret_cast<t_goto_entry*>(base+hdr->goto_entries_offset);
    t_rule_entry* rule=reinterpret_cast<t_rule_entry*>(base+hdr->rule_entries_offset);
    rule->lhs=CFG_FIRST_NON_TERMINAL;rule->rhs_count=1;rule->semantic_id=PARSING_NO_SEMANTIC;
    offset=0;
    for(size_t s=0;s<states;++s) {
        spans[s].offset=offset;spans[s].count=sizes[s];spans[s].default_action=pack_action(ACTION_ERROR,0);
        if(s%2) spans[s].default_action=pack_action(ACTION_REDUCE,0);
        gotos[s].offset=offset;gotos[s].count=sizes[s];
        for(size_t i=0;i<sizes[s];++i) {
            actions[offset+i].terminal=i*257/sizes[s];actions[offset+i].action=pack_action(ACTION_SHIFT,(s+i)%states);
            targets[offset+i].non_terminal=i*257/sizes[s];targets[offset+i].target=(s+i)%states;
        }
        offset+=sizes[s];
    }
    TableView view(base,total);size_t checked=0;
    for(size_t s=0;s<states;++s) for(size_t symbol=0;symbol<=256;++symbol) {
        t_packed_action expected=spans[s].default_action;t_id_state target=PARSING_INVALID_GOTO;
        for(size_t i=0;i<sizes[s];++i) if(actions[spans[s].offset+i].terminal==symbol) {
            expected=actions[spans[s].offset+i].action;target=targets[gotos[s].offset+i].target;
        }
        t_action actual=view.action(s,symbol);
        if(actual.type!=unpack_action_type(expected)||actual.value!=unpack_action_value(expected))return 1;
        if(view.goto_state(s,CFG_FIRST_NON_TERMINAL+symbol)!=target)return 2;
        checked+=2;
    }
    bool low=false,high=false;
    try {view.goto_state(0,0);}catch(const std::out_of_range&){low=true;}
    try {view.goto_state(0,CFG_FIRST_NON_TERMINAL+257);}catch(const std::out_of_range&){high=true;}
    if(!low||!high||view.rule(0).lhs!=rule->lhs||view.rule(0).semantic_id!=PARSING_NO_SEMANTIC)return 3;
    std::cout<<checked<<" lookup results match oracle; nonterminal bounds and rule metadata pass\n";
}
