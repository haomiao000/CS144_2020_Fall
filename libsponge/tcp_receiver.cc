#include "tcp_receiver.hh"
#include<iostream>
// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.
// TCP接收方的虚拟实现

// 对于实验2，请用能够通过`make check_lab2`运行的真实实现替换它。
// 如果需要，设置初始序列号。具有SYN标志集的第一个到达的段的序列号是初始序列号。
// 您需要跟踪这一点，以便在32位包装的seqnos/acknos和它们的绝对等价值之间进行转换。
// （请注意，SYN标志只是标头中的一个标志。同一个段也可能携带数据，甚至可能设置FIN标志。）
// 将任何数据或流结束标记推送到StreamReassembler。如果TCPSegment的标头中设置了FIN标志，
// 这意味着有效负载的最后一个字节是整个流的最后一个字节。请记住，StreamReassembler期望从零开始的流索引；
// 您将不得不取消包装seqnos以产生这些索引。
template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    // cout<<seg.header().seqno<<" "<<seg.header().syn<<" "<<seg.header().fin<<" "<<endl;
    // cout<<_isn.has_value()<<" "<<seg.header().syn<<" "<<seg.header().seqno<<" "<<seg.payload().copy()<<endl;
    if(!_isn.has_value()){
        if(seg.header().syn==true){
            _isn=WrappingInt32(seg.header().seqno);
        }
        else{
            return;
        }
    }
    uint64_t abs_no = unwrap(seg.header().seqno , _isn.value() , _reassembler.stream_out().bytes_written());
    // cout<<seg.payload().copy()<<endl;
    // cout<<"abs="<<abs_no<<endl;
    _reassembler.push_substring(seg.payload().copy() , (seg.header().syn==true)?0:(abs_no-1) , seg.header().fin);
    // cout<<seg.header().fin<<"---"<<endl;
    // cout<<"wr="<<_reassembler.stream_out().bytes_written()<<" ed_pos="<<_reassembler.getReassemblerEndPos()<<" fin="<<seg.header().fin<<" "<<_reassembler.stream_out().input_ended()<<" ifend="<<_reassembler.getReassemblerIFEnd()<<endl;
    // cout<<seg.header().fin<<" "<<_reassembler.stream_out().input_ended()<<endl;
    _ackno=wrap((_reassembler.stream_out().bytes_written()+((_reassembler.stream_out().input_ended())?2:1)) , _isn.value());
    // cout<<endl;
}

optional<WrappingInt32> TCPReceiver::ackno() const { return _ackno; }

size_t TCPReceiver::window_size() const { return _capacity-_reassembler.stream_out().buffer_size(); }
