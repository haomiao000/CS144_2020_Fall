#include "tcp_sender.hh"

#include "tcp_config.hh"
#include<iostream>
#include <random>
// Dummy implementation of a TCP sender
// 虚构的TCP发送方实现
// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.
// 在Lab 3中，请替换为通过`make check_lab3`运行的自动检查的真实实现。
template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] capacity 传出字节流的容量
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] retx_timeout 重新传输最老的未确认段之前等待的初始时间量
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
//! \param[in] fixed_isn 要使用的初始序列号（如果设置）（否则使用随机的ISN）
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) 
    , _window_size(1)
    , _ackno_now{0}
    , bytes_has_sent(0)
    ,_num_of_capacity(0)
    ,if_zero_window(false)
    ,_ms_total_tick(0)
    ,_consecutive_retransmissions(0)
    ,_retransmission_timeout(_initial_retransmission_timeout)
    ,_outgoing_seg{}
    ,bytes_flight(0)
    ,_fin(0)
    {}
uint64_t TCPSender::bytes_in_flight() const { return bytes_flight; }

void TCPSender::fill_window() {
    
    // (!_window_size) ? (_window_size=1 , if_zero_window=true) : (if_zero_window=false);
    while(_window_size>bytes_flight){
        // cout<<"fillstart"<<"----------"<<endl;
        TCPSegment seg;
        size_t tmp_syn;
        //syn
        if(_next_seqno==0) {
            seg.header().syn=true;
            _num_of_capacity=_stream.buffer_size();
            tmp_syn=1;
        }else tmp_syn=0;
        size_t fill_sz=min(_window_size-tmp_syn-bytes_flight , min(TCPConfig::MAX_PAYLOAD_SIZE , _stream.buffer_size()));
        //payload
        // cout<<fill_sz<<endl;
        seg.payload()=Buffer(_stream.read(fill_sz));
        //seqno
        // cout<<seg.payload().copy()<<endl;
        seg.header().seqno=wrap(_next_seqno , _isn);
        //fin
        if(_stream.eof()&&fill_sz+tmp_syn<_window_size-bytes_flight&&!_fin){
            seg.header().fin=1;
            _fin=true;
        }
        bytes_flight+=seg.length_in_sequence_space();
        if(seg.length_in_sequence_space()==0) {
            // cout<<"fillend"<<"----------"<<endl;
            break;
        }
        _segments_out.push(seg);
        // cout<<"head_seq="<<_outgoing_seg[0].header().seqno.raw_value()<<" _outgoing_seg_sz="<<_outgoing_seg.size()<<endl;
        // cout<<"fill_sz="<<fill_sz<<" _stream_readend="<<_stream.input_ended()<<" bytes_in_flight="<<bytes_flight<<endl;
        // cout<<"isn="<<_isn<<" window_sz="<<_window_size<<" out_seg="<<_outgoing_seg.size()<<" ack="<<_ackno_now<<" stream_sz="<<_stream.buffer_size()<<" outque_sz="<<_segments_out.size()<<endl;
        // cout<<"syn="<<seg.header().syn<<" fin="<<seg.header().fin<<" seqno="<<seg.header().seqno.raw_value()<<" payload="<<seg.payload().copy()<<endl;
        // cout<<"fillend"<<"----------"<<endl;
        _outgoing_seg.push_back(seg);
        //_next_seqno
        (seg.header().syn) ? (_next_seqno+=1) : (_next_seqno+=fill_sz);
        (seg.header().fin) ? (_next_seqno+=1) : (_next_seqno);
        if(_window_size==0||_window_size-bytes_flight<=0||_fin) break;
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param ackno 远程接收方的确认号
//! \param window_size The remote receiver's advertised window size
//! \param window_size 远程接收方的广告窗口大小 
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    _window_size=window_size;
    // cout<<"ackstart-----------------ack="<<ackno.raw_value()<<endl;
    uint64_t nw_ack=unwrap(ackno , _isn , _ackno_now);
    // cout<<"nw_ack="<<nw_ack<<" ack_now="<<_ackno_now<<endl;
    if(nw_ack>_ackno_now){
        while(!_outgoing_seg.empty()&&_ackno_now+_outgoing_seg[0].length_in_sequence_space()<=nw_ack){
            // cout<<"pop()"<<endl;
            bytes_flight-=_outgoing_seg[0].length_in_sequence_space();
            _ackno_now+=_outgoing_seg[0].length_in_sequence_space();
            _outgoing_seg.pop_front();
        }
        // cout<<"_outgoing_seg_sz="<<_outgoing_seg.size()<<" "<<"after_receive_bytesflight="<<bytes_flight<<endl;
        _consecutive_retransmissions=0 , _ms_total_tick=0;
        _retransmission_timeout=_initial_retransmission_timeout;        
    }
    (!_window_size) ? (_window_size=1 , if_zero_window=true) : (if_zero_window=false);  
    // cout<<"ackend-----------------"<<endl;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
//! \param[in] ms_since_last_tick 距离上一次调用此方法的毫秒数
void TCPSender::tick(const size_t ms_since_last_tick) {
    // cout<<"tickstart_with_ms="<<_ms_total_tick<<"!!!!!!!!!!!!!!!!!!!!!_retransmission_timeout="<<_retransmission_timeout<<endl;
    // size_t cnt=_ms_total_tick;
    _ms_total_tick+=ms_since_last_tick;
    // cout<<"tickstart_with_ms="<<_ms_total_tick<<"!!!!!!!!!!!!!!!!!!!!!"<<endl;
    if(_ms_total_tick>=_retransmission_timeout){
        if(!_outgoing_seg.empty()){
            _segments_out.push(_outgoing_seg[0]);
            TCPSegment seg=_outgoing_seg[0];
            // cout<<"ticksend-------_ms_total_tick="<<cnt<<" _retransmission_timeout="<<_retransmission_timeout<<" ms_since_last_tick="<<ms_since_last_tick<<" zerowindow="<<if_zero_window<<endl;
            // cout<<"ticksend-------isn="<<_isn<<" window_sz="<<_window_size<<" out_seg="<<_outgoing_seg.size()<<" ack="<<_ackno_now<<" stream_sz="<<_stream.buffer_size()<<" outque_sz="<<_segments_out.size()<<endl;
            // cout<<"ticksend-------syn="<<seg.header().syn<<" fin="<<seg.header().fin<<" seqno="<<seg.header().seqno.raw_value()<<" payload="<<seg.payload().copy()<<endl;
        }
        if(!if_zero_window){
            _ms_total_tick=0;
            _retransmission_timeout*=2;
        }else _ms_total_tick=0;
        _consecutive_retransmissions++;
    }
    // cout<<"tickend!!!!!!!!!!!!!!!!!!!!!"<<endl;
}

unsigned int TCPSender::consecutive_retransmissions() const {return _consecutive_retransmissions;}

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno=_isn;
    _segments_out.push(seg);
}
