#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`
#include<iostream>
template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : 
    _output(capacity), 
    _capacity(capacity), 
    reassembler_buf(),
    reassembler_if_end(false),
    reassembler_end_pos(0){
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // cout<<data<<" "<<index<<" "<<eof<<endl;
    size_t index_maxpos=_output.bytes_read()+_capacity-1;
    size_t datalen=data.length();
    if(index_maxpos<index){
        return;
    }
    if(index+datalen+1<=_output.bytes_written()){
        return;
    }
    if(eof==true){
        reassembler_if_end=true;
        reassembler_end_pos=index+datalen;
    }

    auto choose_st=[&](size_t indx) -> size_t{
        return (_output.bytes_written()<indx+1)?0:(_output.bytes_written()-indx);
    };
    for(size_t i=choose_st(index);i<datalen&&index+i<=index_maxpos;i++){
        Element tmp = {index+i , data[i]};
        reassembler_buf.insert(tmp);
    }
    
    string write_subs="";
    int tag=0;
    while(!reassembler_buf.empty()&&(*reassembler_buf.begin()).pos==_output.bytes_written()+tag){
        // cout<<(*reassembler_buf.begin()).value<<"******"<<endl;
        write_subs.push_back((*reassembler_buf.begin()).value);
        reassembler_buf.erase(reassembler_buf.begin());
        tag++;
    }
    
    if(write_subs!=""){
        _output.write(write_subs);
    }
    // cout<<_output.bytes_written()<<" "<<reassembler_end_pos<<" "<<reassembler_if_end<<endl;
    if(_output.bytes_written()==reassembler_end_pos&&reassembler_if_end==true){
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return reassembler_buf.size(); }

bool StreamReassembler::empty() const { return reassembler_buf.empty(); }
