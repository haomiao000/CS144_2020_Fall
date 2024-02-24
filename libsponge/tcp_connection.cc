#include "tcp_connection.hh"
#include <iostream>
#include <limits>
#define DG "[debug]  "
#define _S_OUT _sender.segments_out()
// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return time_now-last_time_receive_seg; }

void TCPConnection::fill_seg(){
    // cout<<DG<<"is in filling"<<endl;
        // cout<<"@@@@@@@@@@@@@@@"<<endl;
    while(!_sender.segments_out().empty()){
        // cout<<"@@@@@@@@@@@@@@@"<<endl;
        auto temp=_sender.segments_out().front();
        // cout<<"@@@@@@@@@@@@@@@"<<endl;
        if(_receiver.ackno().has_value()) temp.header().ackno=_receiver.ackno().value();
        // cout<<"@@@@@@@@@@@@@@@"<<endl;
        if(_receiver.ackno().has_value()) temp.header().ack=true;
        // cout<<"@@@@@@@@@@@@@@@"<<endl;
        temp.header().win=min(_receiver.window_size() , numeric_limits<size_t>::max());
        // cout<<"@@@@@@@@@@@@@@@"<<endl;
        _segments_out.push(temp);
        // cout<<"@@@@@@@@@@@@@@@"<<endl;
        _sender.segments_out().pop();    
        // cout<<"@@@@@@@@@@@@@@@"<<endl;   
        // cout<<"sending seg:";debug_seg(temp);
    }
}

void TCPConnection::fill_rst(){
    
    // cout<<"filling_rest with senderseg_size="<<_sender.segments_out().size()<<"  seg_size()="<<_segments_out.size()<<endl;
    while(!_segments_out.empty())_segments_out.pop();
    auto temp=_sender.segments_out().front();
    temp.header().rst=true;
    if(_receiver.ackno().has_value()) temp.header().ackno=_receiver.ackno().value();
    // cout<<"rst_seg:";debug_seg(temp);
    _segments_out.push(temp);
    _sender.segments_out().pop();
}

void TCPConnection::set_errro(){
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
}

// void TCPConnection::debug_seg(TCPSegment seg){
    // cout<<"  syn="<<((seg.header().syn)?"true":"false")<<"  fin="<<((seg.header().fin)?"true":"false")<<endl<<"   "
                                //    <<"  seqno="<<seg.header().seqno<<"  ack="<<seg.header().ack<<"  ackno="<<seg.header().ackno<<"  rst="<<seg.header().rst<<endl<<"   "
                                //    <<"  segspace="<<seg.length_in_sequence_space()<<"  segdata="<<"seg.payload().copy()"<<"  win="<<seg.header().win<<endl;
// }
// void TCPConnection::debug_senderque(){
//     std::queue<TCPSegment>debug_que;
//     // int count=0;
//     while(!_S_OUT.empty()){
//         auto temp=_S_OUT.front();
//         // cout<<"seg "<<++count<<" ";
//         debug_seg(temp);
//         debug_que.push(temp);
//         _S_OUT.pop();
//     }
//     while(!debug_que.empty()){
//         _S_OUT.push(debug_que.front());
//         debug_que.pop();
//     }
// }

// void TCPConnection::debug_outque(){
//     std::queue<TCPSegment>debug_que;
//     // int count=0;
//     while(!segments_out().empty()){
//         auto temp=segments_out().front();
//         // cout<<"seg "<<++count<<" ";
//         debug_seg(temp);
//         debug_que.push(temp);
//         segments_out().pop();
//     }
//     while(!debug_que.empty()){
//         segments_out().push(debug_que.front());
//         debug_que.pop();
//     }    
// }
void TCPConnection::segment_received(const TCPSegment &seg) {
    
    // cout<<DG<<"[segment_received]"<<"segment_received seg at time "<<time_now<<" :";debug_seg(seg);
    // cout<<"_active=  "<<((_active)?"true":"false")<<endl;
    if(!_active){
        return;
    }
    //1. 如果RST（reset）标志位为真，将发送端stream和接受端stream设置成error state并终止连接。
    if(seg.header().rst){
        // cout<<"DG"<<"[RST]received seg with rst and close"<<endl;
        _active=false;
        set_errro();
        return;
    }
    //2. 把segment传递给TCPReceiver，这样的话，TCPReceiver就能从segment取出它所关心的字段进行处理了：seqno，SYN，payload，FIN。
    _receiver.segment_received(seg);
    last_time_receive_seg=time_now;
    //3. 如果ACK标志位为真，通知TCPSender有segment被确认，TCPSender关心的字段有ackno和window_size。
    if(seg.header().ack){
        _sender.ack_received(seg.header().ackno , seg.header().win);
        if(_send_syn){
            _sender.fill_window();
            fill_seg();
        }
    }
    //4. 如果收到的segment不为空，TCPConnection必须确保至少给这个segment回复一个ACK，以便远端的发送方更新ackno和window_size。
    if(seg.length_in_sequence_space()){
        // cout<<"response ack"<<endl;
        if(seg.header().syn&&!seg.header().ack&&!_send_syn){
            _send_syn=1;
            _sender.fill_window();
        }else {
            _sender.send_empty_segment(); 
        }
        fill_seg();
    }
    //5.  “keep-alive” 
    // 在发送segment之前，
    // TCPConnection会读取TCPReceiver中关于ackno和window_size相关的信息。
    // 如果当前TCPReceiver里有一个合法的ackno，
    // TCPConnection会更改TCPSegment里的ACK标志位，
    // 将其设置为真。
    if (_receiver.ackno().has_value() and (seg.length_in_sequence_space() == 0)
        and seg.header().seqno == _receiver.ackno().value() - 1) {
        // cout<<DG<<"[keep-alive] going"<<endl;
        _sender.send_empty_segment();
        fill_seg();
    }
    if(!_sender.stream_in().eof()&&_receiver.stream_out().input_ended()){
        // cout<<DG<<" _linger set false"<<endl;
        _linger_after_streams_finish=false;
    }
    if(_receiver.stream_out().eof()&&_sender.stream_in().input_ended()&&bytes_in_flight()==0){
        // cout<<"here makes active false"<<endl;
        if(!_linger_after_streams_finish){
            _active=false;
        }
    }
}

bool TCPConnection::active() const {
    return _active;
}

size_t TCPConnection::write(const string &data) {
    // cout<<DG<<"[write]"<<"write going="<<"data"<<"  writing active="<<((_active)?"true":"false")<<endl;
    if(!_active){
        return 0;
    }
   
    size_t datalen=_sender.stream_in().write(data);

    _sender.fill_window();
    fill_seg();

    return datalen;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    // cout<<DG<<"here is wrong"<<endl;
    if(!_active){
        return;
    }
    _sender.tick(ms_since_last_tick);
    fill_seg();
    //1. 告诉TCPSender时间正在流逝。
    time_now+=ms_since_last_tick;
    // cout<<DG<<"[tick]"<<"tick going="<<ms_since_last_tick<<"  now_time="<<time_now<<"  last_seg_recv="<<last_time_receive_seg<<"  bytes_in_flight="<<bytes_in_flight()<<"------>time-out="<<_cfg.rt_timeout<<endl;
    //2. 如果同一个segment连续重传的次数超过TCPConfig::MAX_RETX_ATTEMPTS，终止连接，并且给对方发送一个reset segment（一个RST为真的空segment）。
    if(_sender.consecutive_retransmissions()>TCPConfig::MAX_RETX_ATTEMPTS){
        // cout<<DG<<"retransmissions too much!!"<<" "<<"->set an error and fill rst"<<endl;
        _active=false;
        _sender.send_empty_segment();
        fill_rst();
        set_errro();
    // cout<<DG<<"after fill_seg connect senderque has"<<endl;
    // debug_senderque();
    // cout<<DG<<"after fill_seg connect segout has"<<endl;
    // debug_outque();        
        return;
    }
    if(_receiver.stream_out().input_ended()&&_sender.stream_in().input_ended()){
        if(_linger_after_streams_finish){
            if(10* _cfg.rt_timeout<=time_since_last_segment_received()){
                // cout<<DG<<" nice close happened with 10*time-out"<<endl;
                _active=false;
            }
        }
    }
    // cout<<"when tick end _sender has consecutive_retransmissions="<<_sender.consecutive_retransmissions()<<endl;
}

void TCPConnection::end_input_stream() {
    // cout<<DG<<"[end_input_stream]"<<"end_input_stream going"<<endl;
    _sender.stream_in().end_input();
    _sender.fill_window();
    fill_seg();
}

void TCPConnection::connect() {
    // cout<<DG<<"[connect]"<<"connect was calling with _active="<<((_active)?"true":"false")<<" and sg_out_size()="<<segments_out().size()<<endl;
    if(!_active){
        return;
    }
    // cout<<DG<<"while calling connect segment_out has"<<endl;
    // debug_outque();
    if(!_send_syn) _send_syn=1;
    _sender.fill_window();
    // cout<<DG<<"after sender.fill_window connect senderque has"<<endl;
    // debug_senderque();
    fill_seg();
    // cout<<DG<<"after fill_seg connect senderque has"<<endl;
    // debug_senderque();
    // cout<<DG<<"after fill_seg connect segout has"<<endl;
    // debug_outque();
}

TCPConnection::~TCPConnection() {
    try {
        // cout<<DG<<"~TCPConnection was called"<<" while _active="<<((_active)?"true":"false")<<endl;
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            while(!_S_OUT.empty()) _S_OUT.pop();
            while(!segments_out().empty()) segments_out().pop();
            _active=false;
            _sender.send_empty_segment();
            // cout<<DG<<" _send_segment_out.size()="<<_sender.segments_out().size()<<endl;
            // debug_senderque();
            fill_rst();
            set_errro();
            while(!_sender.segments_out().empty()) _S_OUT.pop();
            while(!segments_out().empty()) segments_out().pop();
            
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
