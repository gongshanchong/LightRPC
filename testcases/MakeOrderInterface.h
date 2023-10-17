#ifndef LIGHTRPC_TESTCASES_MAKEORDERINTERFACE_H
#define LIGHTRPC_TESTCASES_MAKEORDERINTERFACE_H

#include "../lightrpc/net/rpc/rpc_interface.h"
#include "order.pb.h"

namespace lightrpc {

class MakeOrderInterface : public RpcInterface{
public:
    void Run(){
        LOG_APPDEBUG("start sleep 5s");
        sleep(5);
        LOG_APPDEBUG("end sleep 5s");
        if (dynamic_cast<const makeOrderRequest*>(m_req_base_)->price() < 10) {
            dynamic_cast<makeOrderResponse*>(m_rsp_base_)->set_ret_code(-1);
            dynamic_cast<makeOrderResponse*>(m_rsp_base_)->set_res_info("short balance");
        return;
        }
        dynamic_cast<makeOrderResponse*>(m_rsp_base_)->set_order_id("20230514");
        LOG_APPDEBUG("call makeOrder success");
    }
};

}

#endif