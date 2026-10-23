// Compare the optimized selector with an independent slow frequency oracle.
#include "internal/table/Table/TableBuilder.hpp"
#include <iostream>
#include <vector>
int main(){
 unsigned int seed=5234;
 for(size_t test=0;test<300;++test){
  TableIR ir(1,1);std::vector<t_action> row(CFG_END+1);
  for(size_t i=0;i<row.size();++i){
   seed=1664525u*seed+1013904223u;
   t_action a;a.type=ACTION_SHIFT;a.value=0;
   if(test==0){a.type=ACTION_ERROR;a.value=0;}
   else if(test==1){a.value=i;}
   else if(test==2){a.value=(i==256?2:i%2);}
   else if(test==3){a.value=(i==256?2:(i+1)%2);}
   else {a.type=(e_action_type)(seed%4);a.value=(seed>>8)%(1+test%257);}
   row[i]=a;ir.set_action(0,i,a);
  }
  t_action expected=row[0];size_t maximum=0;
  for(size_t i=0;i<row.size();++i){size_t count=0;for(size_t j=0;j<row.size();++j)if(same_action(row[i],row[j]))++count;
   if(count>maximum){maximum=count;expected=row[i];}}
  t_action actual=find_default_action(ir,0);
  if(!same_action(actual,expected)){std::cerr<<"mismatch row "<<test<<'\n';return 1;}
 }
 std::cout<<"300 rows: uniform, all distinct, ties and seeded frequencies match oracle\n";
}
