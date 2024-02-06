#ifndef SPONGE_LIBSPONGE_TCP_RECEIVER_HH
#define SPONGE_LIBSPONGE_TCP_RECEIVER_HH

#include "byte_stream.hh"
#include "stream_reassembler.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <optional>

//! \brief The "receiver" part of a TCP implementation.
//! \brief TCP实现中的"接收方"部分。


//! Receives and reassembles segments into a ByteStream, and computes
//! the acknowledgment number and window size to advertise back to the
//! remote TCPSender.
//! 接收和重组段成为ByteStream，并计算应该回传给远程TCPSender的确认号和窗口大小。
class TCPReceiver {
    //! Our data structure for re-assembling bytes.
    //! 我们用于重新组装字节的数据结构。
    StreamReassembler _reassembler;

    //! The maximum number of bytes we'll store.
    //! 我们将存储的最大字节数。
    size_t _capacity;
    std::optional<WrappingInt32> _ackno;
    std::optional<WrappingInt32> _isn;

  public:
    //! \brief Construct a TCP receiver
    //! \brief 构造一个TCP接收方
    //!
    //! \param capacity the maximum number of bytes that the receiver will
    //!                 store in its buffers at any give time.
    //! \param capacity 接收方将在任何给定时间存储在其缓冲区中的最大字节数。
    TCPReceiver(const size_t capacity) : _reassembler(capacity), _capacity(capacity) , _ackno(std::nullopt) , _isn(std::nullopt){}

    //! \name Accessors to provide feedback to the remote TCPSender
    //!@{

    //! \brief The ackno that should be sent to the peer
    //! \brief 应该发送给对等方的确认号
    //! \returns empty if no SYN has been received
    //! \returns 如果没有收到SYN，则为空
    //!
    //! This is the beginning of the receiver's window, or in other words, the sequence number
    //! of the first byte in the stream that the receiver hasn't received.
    //! 这是接收方窗口的起始，换句话说，接收方尚未接收到的流中第一个字节的序列号。
    std::optional<WrappingInt32> ackno() const;

    //! \brief The window size that should be sent to the peer
    //! \brief 应发送给对等方的窗口大小
    //!
    //! Operationally: the capacity minus the number of bytes that the
    //! TCPReceiver is holding in its byte stream (those that have been
    //! reassembled, but not consumed).
    //! 操作上：容量减去TCPReceiver在其字节流中持有的字节数（已重新组装但尚未使用的字节）。

    //!
    //! Formally: the difference between (a) the sequence number of
    //! the first byte that falls after the window (and will not be
    //! accepted by the receiver) and (b) the sequence number of the
    //! beginning of the window (the ackno).
    //! 形式上：（a）第一个落在窗口之后的字节的序列号（接收方将不接受该字节）和（b）窗口开始的序列号（ackno）之间的差异。

    size_t window_size() const;
    //!@}

    //! \brief number of bytes stored but not yet reassembled
    //! \brief 存储但尚未重新组装的字节数
    size_t unassembled_bytes() const { return _reassembler.unassembled_bytes(); }

    //! \brief handle an inbound segment
    //! \brief 处理传入的段

    void segment_received(const TCPSegment &seg);

    //! \name "Output" interface for the reader
    //!@{
    ByteStream &stream_out() { return _reassembler.stream_out(); }
    const ByteStream &stream_out() const { return _reassembler.stream_out(); }
    //!@}
};

#endif  // SPONGE_LIBSPONGE_TCP_RECEIVER_HH
