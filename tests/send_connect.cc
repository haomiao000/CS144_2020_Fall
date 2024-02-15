#include "sender_harness.hh"
#include "wrapping_integers.hh"

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

using namespace std;

int main() {
    try {
        // cout<<"*****************"<<endl;
        auto rd = get_random_generator();
        {
            // cout<<"1"<<endl;
            TCPConfig cfg;
            WrappingInt32 isn(rd());
            cfg.fixed_isn = isn;
            // cout<<"2"<<endl;
            TCPSenderTestHarness test{"SYN sent test", cfg};
            test.execute(ExpectState{TCPSenderStateSummary::SYN_SENT});
            test.execute(ExpectSegment{}.with_no_flags().with_syn(true).with_payload_size(0).with_seqno(isn));
            test.execute(ExpectBytesInFlight{1});
            // cout<<"3"<<endl;
        }
        // cout<<"*****************"<<endl;
        {
            // cout<<"4"<<endl;
            TCPConfig cfg;
            WrappingInt32 isn(rd());
            cfg.fixed_isn = isn;
            // cout<<"5"<<endl;
            TCPSenderTestHarness test{"SYN acked test", cfg};
            test.execute(ExpectState{TCPSenderStateSummary::SYN_SENT});
            // cout<<"6"<<endl;
            test.execute(ExpectSegment{}.with_no_flags().with_syn(true).with_payload_size(0).with_seqno(isn));
            // cout<<"7"<<endl;
            test.execute(ExpectBytesInFlight{1});
            // cout<<"8"<<endl;
            test.execute(AckReceived{WrappingInt32{isn + 1}});
            // cout<<"9"<<endl;
            test.execute(ExpectState{TCPSenderStateSummary::SYN_ACKED});
            // cout<<"10"<<endl;
            test.execute(ExpectNoSegment{});
            // cout<<"11"<<endl;
            test.execute(ExpectBytesInFlight{0});
            // cout<<"12"<<endl;
        }
        // cout<<"!!!!!!!!!!!"<<endl;
        {
            TCPConfig cfg;
            WrappingInt32 isn(rd());
            cfg.fixed_isn = isn;

            TCPSenderTestHarness test{"SYN -> wrong ack test", cfg};
            test.execute(ExpectState{TCPSenderStateSummary::SYN_SENT});
            test.execute(ExpectSegment{}.with_no_flags().with_syn(true).with_payload_size(0).with_seqno(isn));
            test.execute(ExpectBytesInFlight{1});
            test.execute(AckReceived{WrappingInt32{isn}});
            test.execute(ExpectState{TCPSenderStateSummary::SYN_SENT});
            test.execute(ExpectNoSegment{});
            test.execute(ExpectBytesInFlight{1});
        }
        // cout<<"&&&&&&&&&&&&"<<endl;
        {
            TCPConfig cfg;
            WrappingInt32 isn(rd());
            cfg.fixed_isn = isn;
            // cout<<"1"<<endl;
            TCPSenderTestHarness test{"SYN acked, data", cfg};
            // cout<<"2"<<endl;
            test.execute(ExpectState{TCPSenderStateSummary::SYN_SENT});
            test.execute(ExpectSegment{}.with_no_flags().with_syn(true).with_payload_size(0).with_seqno(isn));
            test.execute(ExpectBytesInFlight{1});
            // cout<<"3"<<endl;
            test.execute(AckReceived{WrappingInt32{isn + 1}});
            test.execute(ExpectState{TCPSenderStateSummary::SYN_ACKED});
            test.execute(ExpectNoSegment{});
            // cout<<"4"<<endl;
            test.execute(ExpectBytesInFlight{0});
            test.execute(WriteBytes{"abcdefgh"});
            test.execute(Tick{1});
            // cout<<"5"<<endl;
            test.execute(ExpectState{TCPSenderStateSummary::SYN_ACKED});
            test.execute(ExpectSegment{}.with_seqno(isn + 1).with_data("abcdefgh"));
            test.execute(ExpectBytesInFlight{8});
            // cout<<"6"<<endl;
            test.execute(AckReceived{WrappingInt32{isn + 9}});
            test.execute(ExpectState{TCPSenderStateSummary::SYN_ACKED});
            test.execute(ExpectNoSegment{});
            // cout<<"7"<<endl;
            test.execute(ExpectBytesInFlight{0});
            test.execute(ExpectSeqno{WrappingInt32{isn + 9}});
        }

    } catch (const exception &e) {
        cerr << e.what() << endl;
        return 1;
    }

    return EXIT_SUCCESS;
}
