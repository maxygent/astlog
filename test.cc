
#include "log_line.h"
int main(){


  TRACE("this is a Trace {:^20}" ,111);
  DEBUG("this is a Debug {:^20}" ,222);
  INFO("this is a Info {}",333);
  WARN("this is a Warn ");
  ERROR("this is a Error {:^20}",555);
  FATAL("this is a Fatal,{:^20} ",666);
}

