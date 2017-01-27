#include <trantor/net/TcpConnection.h>
#include <trantor/net/Socket.h>
#include <trantor/net/Channel.h>
#define FETCH_SIZE 2048;
using namespace trantor;
TcpConnection::TcpConnection(EventLoop *loop, int socketfd,const InetAddress& localAddr,
                             const InetAddress& peerAddr):
        loop_(loop),
        ioChennelPtr_(new Channel(loop,socketfd)),
        socketPtr_(new Socket(socketfd)),
        localAddr_(localAddr),
        peerAddr_(peerAddr),
        state_(Connecting)
{
    LOG_TRACE<<"new connection:"<<peerAddr.toIpPort()<<"->"<<localAddr.toIpPort();
    ioChennelPtr_->setReadCallback(std::bind(&TcpConnection::readCallback,this));
}
TcpConnection::~TcpConnection() {

}
void TcpConnection::readCallback() {
    //LOG_TRACE<<"read Callback";
    loop_->assertInLoopThread();
    int ret=0;

    size_t n=readBuffer_.readFd(socketPtr_->fd(),&ret);
    //LOG_TRACE<<"read "<<n<<" bytes from socket";
    if(n==0)
    {
        //socket closed by peer
        handleClose();
    }
    else if(n<0)
    {
        LOG_SYSERR<<"read socket error";
    }

    if(n>0&&recvMsgCallback_)
    {
        recvMsgCallback_(shared_from_this(),&readBuffer_);
    }
}
void TcpConnection::connectEstablished() {
    loop_->assertInLoopThread();
    assert(state_==Connecting);
    ioChennelPtr_->tie(shared_from_this());
    ioChennelPtr_->enableReading();
    state_=Connected;
    if(connectionCallback_)
        connectionCallback_(shared_from_this());
}
void TcpConnection::handleClose() {
    LOG_TRACE<<"connection closed";
    loop_->assertInLoopThread();
    state_=Disconnected;
    ioChennelPtr_->disableAll();
    auto guardThis=shared_from_this();
    if(connectionCallback_)
        connectionCallback_(guardThis);
    if(closeCallback_)
    {
        LOG_TRACE<<"to call close callback";
        closeCallback_(guardThis);
    }

}

void TcpConnection::connectDestroyed()
{
    loop_->assertInLoopThread();
    if (state_ == Connected)
    {
        state_=Disconnected;
        ioChennelPtr_->disableAll();

        connectionCallback_(shared_from_this());
    }
    ioChennelPtr_->remove();
}