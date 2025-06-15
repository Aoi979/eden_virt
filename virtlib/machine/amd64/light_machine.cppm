//
// Created by aoikajitsu on 25-6-15.
//
module;
export module machine:amd64;
import eden_machine;
export namespace eden_virt::machine::amd64 {
    template<typename hypervisor>
    struct light_machine : eden_machine<hypervisor> {

    };
}