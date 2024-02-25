#ifndef SPONGE_LIBSPONGE_TCP_FACTORED_HH
#define SPONGE_LIBSPONGE_TCP_FACTORED_HH

#include "tcp_config.hh"
#include "tcp_receiver.hh"
#include "tcp_sender.hh"
#include "tcp_state.hh"

//! \brief A complete endpoint of a TCP connection
//! \brief TCP连接的完整端点
class TCPConnection {
  private:
    TCPConfig _cfg;
    TCPReceiver _receiver{_cfg.recv_capacity};
    TCPSender _sender{_cfg.send_capacity, _cfg.rt_timeout, _cfg.fixed_isn};

    //! outbound queue of segments that the TCPConnection wants sent
    //! 要发送的TCP连接的输出段队列    
    std::queue<TCPSegment> _segments_out{};

    //! Should the TCPConnection stay active (and keep ACKing)
    //! for 10 * _cfg.rt_timeout milliseconds after both streams have ended,
    //! in case the remote TCPConnection doesn't know we've received its whole stream?
    //! 在两个流结束后，TCPConnection是否应保持活动状态（并继续发送ACK）
    //! 以防远程TCPConnection不知道我们已经接收了它的整个流？
    bool _linger_after_streams_finish{true};

    size_t time_now{0};

    size_t last_time_receive_seg{0};

    bool _active{true};

    bool _send_syn{false};


  public:
    //! \name "Input" interface for the writer
    //! \name 用于写入的"Input"接口
    //!@{

    //! \brief Initiate a connection by sending a SYN segment
    //! 通过发送SYN段来初始化连接
    void connect();

    //! \brief Write data to the outbound byte stream, and send it over TCP if possible
    //! \returns the number of bytes from `data` that were actually written.
    //! 将数据写入输出字节流，并在可能的情况下通过TCP发送
    //! 返回实际写入的`data`字节的数量。
    size_t write(const std::string &data);

    //! \returns the number of `bytes` that can be written right now.
    //! 返回当前可以写入的字节数。
    size_t remaining_outbound_capacity() const;

    //! \brief Shut down the outbound byte stream (still allows reading incoming data)
    //! 关闭输出字节流（仍允许读取传入数据）
    void end_input_stream();
    //!@}

    //! \name "Output" interface for the reader
    //! \name 用于读取的"Output"接口
    //!@{

    //! \brief The inbound byte stream received from the peer
    ByteStream &inbound_stream() { return _receiver.stream_out(); }
    //!@}

    //! \name Accessors used for testing
    //! \name 用于测试的访问器

    //!@{
    //! \brief number of bytes sent and not yet acknowledged, counting SYN/FIN each as one byte
    //! 已发送但尚未确认的字节数，每个SYN/FIN计为一个字节
    size_t bytes_in_flight() const;
    //! \brief number of bytes not yet reassembled
    //! 尚未重新组装的字节数
    size_t unassembled_bytes() const;
    //! \brief Number of milliseconds since the last segment was received
    //! 自上次接收段以来的毫秒数
    size_t time_since_last_segment_received() const;
    //!< \brief summarize the state of the sender, receiver, and the connection
    //!< 摘要发送方、接收方和连接的状态
    TCPState state() const { return {_sender, _receiver, active(), _linger_after_streams_finish}; };
    //!@}

    //! \name Methods for the owner or operating system to call
    //! \name 用于所有者或操作系统调用的方法

    //!@{
    void fill_seg();

    void fill_rst();

    void set_errro();

    void debug_seg(const TCPSegment seg);

    void debug_senderque();

    void debug_outque();


    //! Called when a new segment has been received from the network
    //! 当从网络接收到新段时调用
    void segment_received(const TCPSegment &seg);

    //! Called periodically when time elapses
    //! 时间流逝时定期调用
    void tick(const size_t ms_since_last_tick);

    //! \brief TCPSegments that the TCPConnection has enqueued for transmission.
    //! \note The owner or operating system will dequeue these and
    //! put each one into the payload of a lower-layer datagram (usually Internet datagrams (IP),
    //! but could also be user datagrams (UDP) or any other kind).
    //! TCPConnection已排队等待传输的TCPSegments。
    //! 注意，所有者或操作系统将出队这些段并
    //! 将每个放入较低层数据报（通常是Internet数据报（IP）的负载，
    //! 但也可能是用户数据报（UDP）或任何其他类型的负载）。
    std::queue<TCPSegment> &segments_out() { return _segments_out; }

    //! \brief Is the connection still alive in any way?
    //! \returns `true` if either stream is still running or if the TCPConnection is lingering
    //! after both streams have finished (e.g. to ACK retransmissions from the peer)
    //! 连接是否仍然以任何方式保持活动？
    //! 如果任一流仍在运行或TCPConnection正在等待
    //! 在两个流结束后保持活动状态（例如为了ACK对等方的重传）
    bool active() const;
    //!@}

    //! Construct a new connection from a configuration
    //! 从配置构建新连接
    explicit TCPConnection(const TCPConfig &cfg) : _cfg{cfg} {}

    //! \name construction and destruction
    //! moving is allowed; copying is disallowed; default construction not possible
    //! \name 构造和析构
    //! 允许移动; 不允许复制; 无法进行默认构造
    //!@{
    ~TCPConnection();  //!< destructor sends a RST if the connection is still open//!< 析构函数如果连接仍然打开，则发送RST
    TCPConnection() = delete;
    TCPConnection(TCPConnection &&other) = default;
    TCPConnection &operator=(TCPConnection &&other) = default;
    TCPConnection(const TCPConnection &other) = delete;
    TCPConnection &operator=(const TCPConnection &other) = delete;
    //!@}
};

#endif  // SPONGE_LIBSPONGE_TCP_FACTORED_HH
