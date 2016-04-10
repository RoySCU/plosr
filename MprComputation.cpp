///
/// \brief Computates MPR set of a node following \RFC{3626} hints.
///
void
RoutingProtocol::MprComputation ()
{
  NS_LOG_FUNCTION (this);

  // MPR computation should be done for each interface. See section 8.3.1
  // (RFC 3626) for details.
  MprSet mprSet;

  // N is the subset of neighbors of the node, which are
  // neighbor "of the interface I"
  NeighborSet N;
  //远距离邻居节点
  NeighborSet N_FAR;
  for (NeighborSet::const_iterator neighbor = m_state.GetNeighbors ().begin ();
       neighbor != m_state.GetNeighbors ().end (); neighbor++)
    {
      if (neighbor->status == NeighborTuple::STATUS_SYM) // I think that we need this check
      {
        //超过200 则存入另一个邻居集
        if(NextPositionDistance(*neighbor)){
          N_FAR.push_back (*neighbor);
        }
          //std::cout<<"distance: "<<NextPositionDistance(*neighbor)<<std::endl;
        N.push_back (*neighbor);
      }
    }

  // N2 is the set of 2-hop neighbors reachable from "the interface
  // I", excluding:
  // (i)   the nodes only reachable by members of N with willingness WILL_NEVER
  // (ii)  the node performing the computation
  // (iii) all the symmetric neighbors: the nodes for which there exists a symmetric
  //       link to this node on some interface.
  TwoHopNeighborSet N2;
  for (TwoHopNeighborSet::const_iterator twoHopNeigh = m_state.GetTwoHopNeighbors ().begin ();
       twoHopNeigh != m_state.GetTwoHopNeighbors ().end (); twoHopNeigh++)
    {
      // excluding:
      // (ii)  the node performing the computation
      if (twoHopNeigh->twoHopNeighborAddr == m_mainAddress)
        {
          continue;
        }

      //  excluding:
      // (i)   the nodes only reachable by members of N with willingness WILL_NEVER
      bool ok = false;
      bool test_flag = false;
      for (NeighborSet::const_iterator neigh = N.begin ();
           neigh != N.end (); neigh++)
      {
        if (neigh->neighborMainAddr == twoHopNeigh->neighborMainAddr)
          {
            if (neigh->willingness == OLSR_WILL_NEVER)
              {
                ok = false;
                break;
              }
            else
              {
                ok = true;
                break;
              }
            test_flag = true;
          }
      }
      if(!test_flag)
      {
        /*for (NeighborSet::const_iterator neigh = N_FAR.begin ();
           neigh != N_FAR.end (); neigh++)
        {
          if (neigh->neighborMainAddr == twoHopNeigh->neighborMainAddr)
            {
              if (neigh->willingness == OLSR_WILL_NEVER)
                {
                  ok = false;
                  break;
                }
              else
                {
                  ok = true;
                  break;
                }
              test_flag = true;
            }
        }*/
      }
      if (!ok)
      {
        continue;
      }

      // excluding:
      // (iii) all the symmetric neighbors: the nodes for which there exists a symmetric
      //       link to this node on some interface.
      for (NeighborSet::const_iterator neigh = N.begin ();
           neigh != N.end (); neigh++)
        {
          if (neigh->neighborMainAddr == twoHopNeigh->twoHopNeighborAddr)
            {
              ok = false;
              break;
            }
        }

      if (ok)
        {
          N2.push_back (*twoHopNeigh);
        }
    }

#ifdef NS3_LOG_ENABLE
  {
    std::ostringstream os;
    os << "[";
    for (TwoHopNeighborSet::const_iterator iter = N2.begin ();
         iter != N2.end (); iter++)
      {
        TwoHopNeighborSet::const_iterator next = iter;
        next++;
        os << iter->neighborMainAddr << "->" << iter->twoHopNeighborAddr;
        if (next != N2.end ())
          os << ", ";
      }
    os << "]";
    NS_LOG_DEBUG ("N2: " << os.str ());
  }
#endif  //NS3_LOG_ENABLE

  // 1. Start with an MPR set made of all members of N with
  // N_willingness equal to WILL_ALWAYS
  for (NeighborSet::const_iterator neighbor = N.begin (); neighbor != N.end (); neighbor++)
    {
      if (neighbor->willingness == OLSR_WILL_ALWAYS)
        {
          mprSet.insert (neighbor->neighborMainAddr);
          // (not in RFC but I think is needed: remove the 2-hop
          // neighbors reachable by the MPR from N2)
          CoverTwoHopNeighbors (neighbor->neighborMainAddr, N2);
        }
    }

  // 2. Calculate D(y), where y is a member of N, for all nodes in N.
  // (we do this later)

  // 3. Add to the MPR set those nodes in N, which are the *only*
  // nodes to provide reachability to a node in N2.
  std::set<Ipv4Address> coveredTwoHopNeighbors;
  for (TwoHopNeighborSet::const_iterator twoHopNeigh = N2.begin (); twoHopNeigh != N2.end (); twoHopNeigh++)
    {
      bool onlyOne = true;
      // try to find another neighbor that can reach twoHopNeigh->twoHopNeighborAddr
      for (TwoHopNeighborSet::const_iterator otherTwoHopNeigh = N2.begin (); otherTwoHopNeigh != N2.end (); otherTwoHopNeigh++)
        {
          if (otherTwoHopNeigh->twoHopNeighborAddr == twoHopNeigh->twoHopNeighborAddr
              && otherTwoHopNeigh->neighborMainAddr != twoHopNeigh->neighborMainAddr)
            {
              onlyOne = false;
              break;
            }
        }
      if (onlyOne)
        {
          NS_LOG_LOGIC ("Neighbor " << twoHopNeigh->neighborMainAddr
                                    << " is the only that can reach 2-hop neigh. "
                                    << twoHopNeigh->twoHopNeighborAddr
                                    << " => select as MPR.");

          mprSet.insert (twoHopNeigh->neighborMainAddr);

          // take note of all the 2-hop neighbors reachable by the newly elected MPR
          for (TwoHopNeighborSet::const_iterator otherTwoHopNeigh = N2.begin ();
               otherTwoHopNeigh != N2.end (); otherTwoHopNeigh++)
            {
              if (otherTwoHopNeigh->neighborMainAddr == twoHopNeigh->neighborMainAddr)
                {
                  coveredTwoHopNeighbors.insert (otherTwoHopNeigh->twoHopNeighborAddr);
                }
            }
        }
    }
  // Remove the nodes from N2 which are now covered by a node in the MPR set.
  for (TwoHopNeighborSet::iterator twoHopNeigh = N2.begin ();
       twoHopNeigh != N2.end (); )
    {
      if (coveredTwoHopNeighbors.find (twoHopNeigh->twoHopNeighborAddr) != coveredTwoHopNeighbors.end ())
        {
          // This works correctly only because it is known that twoHopNeigh is reachable by exactly one neighbor, 
          // so only one record in N2 exists for each of them. This record is erased here.
          NS_LOG_LOGIC ("2-hop neigh. " << twoHopNeigh->twoHopNeighborAddr << " is already covered by an MPR.");
          twoHopNeigh = N2.erase (twoHopNeigh);
        }
      else
        {
          twoHopNeigh++;
        }
    }

  // 4. While there exist nodes in N2 which are not covered by at
  // least one node in the MPR set:
  while (N2.begin () != N2.end ())
    {

#ifdef NS3_LOG_ENABLE
      {
        std::ostringstream os;
        os << "[";
        for (TwoHopNeighborSet::const_iterator iter = N2.begin ();
             iter != N2.end (); iter++)
          {
            TwoHopNeighborSet::const_iterator next = iter;
            next++;
            os << iter->neighborMainAddr << "->" << iter->twoHopNeighborAddr;
            if (next != N2.end ())
              os << ", ";
          }
        os << "]";
        NS_LOG_DEBUG ("Step 4 iteration: N2=" << os.str ());
      }
#endif  //NS3_LOG_ENABLE


      // 4.1. For each node in N, calculate the reachability, i.e., the
      // number of nodes in N2 which are not yet covered by at
      // least one node in the MPR set, and which are reachable
      // through this 1-hop neighbor
      std::map<int, std::vector<const NeighborTuple *> > reachability;
      std::set<int> rs;
      for (NeighborSet::iterator it = N.begin (); it != N.end (); it++)
        {
          NeighborTuple const &nb_tuple = *it;
          int r = 0;
          for (TwoHopNeighborSet::iterator it2 = N2.begin (); it2 != N2.end (); it2++)
            {
              TwoHopNeighborTuple const &nb2hop_tuple = *it2;
              if (nb_tuple.neighborMainAddr == nb2hop_tuple.neighborMainAddr)
                r++;
            }
          rs.insert (r);
          reachability[r].push_back (&nb_tuple);
        }

      // 4.2. Select as a MPR the node with highest N_willingness among
      // the nodes in N with non-zero reachability. In case of
      // multiple choice select the node which provides
      // reachability to the maximum number of nodes in N2. In
      // case of multiple nodes providing the same amount of
      // reachability, select the node as MPR whose D(y) is
      // greater. Remove the nodes from N2 which are now covered
      // by a node in the MPR set.
      NeighborTuple const *max = NULL;
      int max_r = 0;
      for (std::set<int>::iterator it = rs.begin (); it != rs.end (); it++)
        {
          int r = *it;
          if (r == 0)
            {
              continue;
            }
          for (std::vector<const NeighborTuple *>::iterator it2 = reachability[r].begin ();
               it2 != reachability[r].end (); it2++)
            {
              const NeighborTuple *nb_tuple = *it2;
              if (max == NULL || nb_tuple->willingness > max->willingness)
                {
                  max = nb_tuple;
                  max_r = r;
                }
              else if (nb_tuple->willingness == max->willingness)
                {
                  if (r > max_r)
                    {
                      max = nb_tuple;
                      max_r = r;
                    }
                  else if (r == max_r)
                    {
                      if (Degree (*nb_tuple) > Degree (*max))
                        {
                          max = nb_tuple;
                          max_r = r;
                        }
                    }
                }
            }
        }

      if (max != NULL)
        {
          mprSet.insert (max->neighborMainAddr);
          CoverTwoHopNeighbors (max->neighborMainAddr, N2);
          NS_LOG_LOGIC (N2.size () << " 2-hop neighbors left to cover!");
        }
    }

#ifdef NS3_LOG_ENABLE
  {
    std::ostringstream os;
    os << "[";
    for (MprSet::const_iterator iter = mprSet.begin ();
         iter != mprSet.end (); iter++)
      {
        MprSet::const_iterator next = iter;
        next++;
        os << *iter;
        if (next != mprSet.end ())
          os << ", ";
      }
    os << "]";
    NS_LOG_DEBUG ("Computed MPR set for node " << m_mainAddress << ": " << os.str ());
  }
#endif  //NS3_LOG_ENABLE

  m_state.SetMprSet (mprSet);
}