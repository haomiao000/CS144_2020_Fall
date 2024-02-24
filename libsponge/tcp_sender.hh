#ifndef SPONGE_LIBSPONGE_TCP_SENDER_HH
#define SPONGE_LIBSPONGE_TCP_SENDER_HH

#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <functional>
#include <queue>
#include <set>
//! \brief The "sender" part of a TCP implementation.
//! \brief TCP实现的“发送方”部分。

//! Accepts a ByteStream, divides it up into segments and sends the
//! segments, keeps track of which segments are still in-flight,
//! maintains the Retransmission Timer, and retransmits in-flight
//! segments if the retransmission timer expires.
//! 接受一个ByteStream，将其分割为段并发送这些段，跟踪仍在传输但尚未被确认的段，
//! 维护重传定时器，并在重传定时器到期时重新传输仍在传输中的段。
class TCPSender {
  private:
    //! our initial sequence number, the number for our SYN.
    //! 我们的初始序列号，用于我们的SYN。
    WrappingInt32 _isn;

    //! outbound queue of segments that the TCPSender wants sent
    //! TCPSender想要发送的输出段的出站队列
    std::queue<TCPSegment> _segments_out{};

    //! retransmission timer for the connection
    //! 连接的重传定时器
    unsigned int _initial_retransmission_timeout;

    //! outgoing stream of bytes that have not yet been sent
    //! 尚未发送的字节的输出流
    ByteStream _stream;

    //! the (absolute) sequence number for the next byte to be sent
    //! 下一个要发送的字节的（绝对）序列号
    uint64_t _next_seqno{0};

    size_t _window_size;

    uint64_t _ackno_now;

    uint64_t bytes_has_sent;

    size_t _num_of_capacity;

    bool if_zero_window;

    size_t _ms_total_tick;

    size_t _consecutive_retransmissions;

    size_t _retransmission_timeout;
    
    std::deque<TCPSegment>_outgoing_seg;

    uint64_t bytes_flight;

    bool _fin;
  public:
    //! Initialize a TCPSender
    //! 初始化一个TCPSender
    TCPSender(const size_t capacity = TCPConfig::DEFAULT_CAPACITY,
              const uint16_t retx_timeout = TCPConfig::TIMEOUT_DFLT,
              const std::optional<WrappingInt32> fixed_isn = {});

    //! \name "Input" interface for the writer
    //! \name 用于写入器的“输入”接口
    //!@{
    ByteStream &stream_in() { return _stream; }
    const ByteStream &stream_in() const { return _stream; }
    //!@}

    //! \name Methods that can cause the TCPSender to send a segment
    //! \name 可以导致TCPSender发送段的方法
    //!@{

    //! \brief A new acknowledgment was received
    //! \brief 收到新的确认
    void ack_received(const WrappingInt32 ackno, const uint16_t window_size);

    //! \brief Generate an empty-payload segment (useful for creating empty ACK segments)
    //! \brief 生成一个空负载段（用于创建空的ACK段）
    void send_empty_segment();

    //! \brief create and send segments to fill as much of the window as possible
    //! \brief 创建并发送段，填充尽可能多的窗口
    void fill_window();

    //! \brief Notifies the TCPSender of the passage of time
    //! \brief 通知TCPSender时间的流逝
    void tick(const size_t ms_since_last_tick);
    //!@}

    //! \name Accessors
    //! \name 访问器
    //!@{

    //! \brief How many sequence numbers are occupied by segments sent but not yet acknowledged?
    //! \note count is in "sequence space," i.e. SYN and FIN each count for one byte
    //! (see TCPSegment::length_in_sequence_space())
    //! \brief 已发送但尚未被确认的段占用多少个序列号？
    //! \note 计数在“序列空间”中，即SYN和FIN每个都计为一个字节
    //! (参见TCPSegment::length_in_sequence_space())
    size_t bytes_in_flight() const;

    //! \brief Number of consecutive retransmissions that have occurred in a row
    //! \brief 连续发生的重传次数
    unsigned int consecutive_retransmissions() const;

    //! \brief TCPSegments that the TCPSender has enqueued for transmission.
    //! \note These must be dequeued and sent by the TCPConnection,
    //! which will need to fill in the fields that are set by the TCPReceiver
    //! (ackno and window size) before sending.
    //! \brief TCPSender已排队等待传输的TCPSegments。
    //! \note 这些必须由TCPConnection出列并发送，
    //! 在发送之前，TCPConnection需要填写由TCPReceiver设置的字段
    //! (ackno和窗口大小)。
    std::queue<TCPSegment> &segments_out() { return _segments_out; }
    //!@}

    //! \name What is the next sequence number? (used for testing)
    //! \name 下一个序列号是什么？（用于测试）

    //!@{

    //! \brief absolute seqno for the next byte to be sent
    //! \brief 下一个要发送的字节的绝对序列号
    uint64_t next_seqno_absolute() const { return _next_seqno; }

    //! \brief relative seqno for the next byte to be sent
    //! \brief 下一个要发送的字节的相对序列号
    WrappingInt32 next_seqno() const { return wrap(_next_seqno, _isn); }
    //!@}
    
};

#endif  // SPONGE_LIBSPONGE_TCP_SENDER_HH
