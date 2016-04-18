#ifndef POLSR_CHCHEDQUEUE_H
#define POLSR_CHCHEDQUEUE_H

#include <vector>
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/simulator.h"


namespace ns3 {
namespace polsr {


class QueueEntry
{
public:
  typedef Ipv4RoutingProtocol::UnicastForwardCallback UnicastForwardCallback;
  typedef Ipv4RoutingProtocol::ErrorCallback ErrorCallback;
  /// c-tor
  QueueEntry (Ptr<Ipv4Route> rentry =0, Ptr<const Packet> pa = 0, Ipv4Header const & h = Ipv4Header (),
              UnicastForwardCallback ucb = UnicastForwardCallback (),
              ErrorCallback ecb = ErrorCallback (), Time exp = Simulator::Now ()) :
    m_rentry(rentry), m_packet (pa), m_header (h), m_ucb (ucb), m_ecb (ecb),
    m_expire (exp + Simulator::Now ())
  {}

  /**
   * Compare queue entries
   * \return true if equal
   */
  bool operator== (QueueEntry const & o) const
  {
    return ((m_rentry == o.m_rentry) && (m_packet == o.m_packet) && (m_header.GetDestination () == o.m_header.GetDestination ()) && (m_expire == o.m_expire));
  }

  // Fields
  UnicastForwardCallback GetUnicastForwardCallback () const { return m_ucb; }
  void SetUnicastForwardCallback (UnicastForwardCallback ucb) { m_ucb = ucb; }
  ErrorCallback GetErrorCallback () const { return m_ecb; }
  void SetErrorCallback (ErrorCallback ecb) { m_ecb = ecb; }
  Ptr<const Packet> GetPacket () const { return m_packet; }
  void SetPacket (Ptr<const Packet> p) { m_packet = p; }
  Ipv4Header GetIpv4Header () const { return m_header; }
  void SetIpv4Header (Ipv4Header h) { m_header = h; }
  void SetExpireTime (Time exp) { m_expire = exp + Simulator::Now (); }
  Time GetExpireTime () const { return m_expire - Simulator::Now (); }
  void SetREntry (Ptr<Ipv4Route> rentry) {m_rentry = rentry;}
  void Send() const { m_ucb(m_rentry,m_packet,m_header); }
private:
  //rentry
  Ptr<Ipv4Route> m_rentry;
  /// Data packet
  Ptr<const Packet> m_packet;
  /// IP header
  Ipv4Header m_header;
  /// Unicast forward callback
  UnicastForwardCallback m_ucb;
  /// Error callback
  ErrorCallback m_ecb;
  /// Expire time for queue entry
  Time m_expire;
};
class CachedQueue
{
public:
  /// Default c-tor
  CachedQueue (uint32_t maxLen, Time routeToQueueTimeout) :
    m_maxLen (maxLen), m_queueTimeout (routeToQueueTimeout)
  {
  }
  /// Push entry in queue, if there is no entry with the same packet and destination address in queue.
  bool Enqueue (QueueEntry & entry);
  /// Return first found (the earliest) entry for given destination
  bool Dequeue (Ipv4Address dst, QueueEntry & entry);
  /// Remove all packets with destination IP address dst
  void DropPacketWithDst (Ipv4Address dst);
  /// Finds whether a packet with destination dst exists in the queue
  bool Find (Ipv4Address dst);
  /// Number of entries
  uint32_t GetSize ();
  void Send();
  void SendOnce();
  // Fields
  uint32_t GetMaxQueueLen () const { return m_maxLen; }
  void SetMaxQueueLen (uint32_t len) { m_maxLen = len; }
  Time GetQueueTimeout () const { return m_queueTimeout; }
  void SetQueueTimeout (Time t) { m_queueTimeout = t; }

private:
  
  std::vector<QueueEntry> m_queue;
  /// Remove all expired entries
  void Purge ();
  /// Notify that packet is dropped from queue by timeout
  void Drop (QueueEntry en, std::string reason);
  /// The maximum number of packets that we allow a routing protocol to buffer.
  uint32_t m_maxLen;
  /// The maximum period of time that a routing protocol is allowed to buffer a packet for, seconds.
  Time m_queueTimeout;
  static bool IsEqual (QueueEntry en, const Ipv4Address dst) { return (en.GetIpv4Header ().GetDestination () == dst); }
};


}
}

#endif /* POLSR_CHCHEDQUEUE_H */
