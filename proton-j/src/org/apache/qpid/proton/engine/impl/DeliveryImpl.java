/*
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 */
package org.apache.qpid.proton.engine.impl;

import org.apache.qpid.proton.engine.Delivery;
import org.apache.qpid.proton.engine.DeliveryState;

public class DeliveryImpl implements Delivery
{
    private DeliveryImpl _linkPrevious;
    private DeliveryImpl _linkNext;
    
    private DeliveryImpl _workNext;
    private DeliveryImpl _workPrev;

    private DeliveryImpl _transportWorkNext;
    private DeliveryImpl _transportWorkPrev;



    private final byte[] _tag;
    private final LinkImpl _link;
    private DeliveryState _deliveryState;
    private boolean _settled;
    private boolean _remoteSettled;
    private DeliveryState _remoteDeliveryState;

    private static final int DELIVERY_STATE_CHANGED = 1;
    private static final int ABLE_TO_SEND = 2;
    private static final int IO_WORK = 4;

    private int _flags = (byte) 0;
    private int _transportFlags = (byte) 0;
    private TransportDelivery _transportDelivery;

    public DeliveryImpl(final byte[] tag, final LinkImpl link, DeliveryImpl previous)
    {
        _tag = tag;
        _link = link;
        _linkPrevious = previous;
        if(previous != null)
        {
            previous._linkNext = this;
        }
    }

    public byte[] getTag()
    {
        return _tag;
    }

    public LinkImpl getLink()
    {
        return _link;
    }

    public DeliveryState getLocalState()
    {
        return _deliveryState;
    }

    public DeliveryState getRemoteState()
    {
        return _remoteDeliveryState;
    }

    public boolean remotelySettled()
    {
        return _remoteSettled;
    }

    public int getMessageFormat()
    {
        return 0;
    }

    public void disposition(final DeliveryState state)
    {
        _deliveryState = state;
        setTransportFlag(DELIVERY_STATE_CHANGED);

    }

    public void settle()
    {
        _settled = true;
    }

    DeliveryImpl getLinkNext()
    {
        return _linkNext;
    }

    public void destroy()
    {
        _link.remove(this);
        if(_linkPrevious != null)
        {
            _linkPrevious._linkNext = _linkNext;
        }
        if(_linkNext != null)
        {
            _linkNext._linkPrevious = _linkPrevious;
        }
    }

    DeliveryImpl getLinkPrevious()
    {
        return _linkPrevious;
    }

    DeliveryImpl getWorkNext()
    {
        return _workNext;
    }

    DeliveryImpl getWorkPrev()
    {
        return _workPrev;
    }


    void setWorkNext(DeliveryImpl workNext)
    {
        _workNext = workNext;
    }

    void setWorkPrev(DeliveryImpl workPrev)
    {
        _workPrev = workPrev;
    }

    int recv(byte[] bytes, int offset, int size)
    {

        //TODO - should only be if no bytes left
        clearFlag(IO_WORK);
        return -1;  //TODO - Implement
    }

    private void clearFlag(int ioWork)
    {
        _flags = _flags & (~IO_WORK);
        if(_flags == 0)
        {
            clearWork();
        }
    }

    void clearWork()
    {
        getLink().getConnectionImpl().removeWork(this);
        if(_workPrev != null)
        {
            _workPrev.setWorkNext(_workNext);
        }
        if(_workNext != null)
        {
            _workNext.setWorkPrev(_workPrev);

        }
        _workNext = null;
        _workPrev = null;
    }

    void addToWorkList()
    {
        getLink().getConnectionImpl().addWork(this);
    }

    void addIOWork()
    {
        setFlag(IO_WORK);
    }

    private void setFlag(int flag)
    {
        boolean addWork;
        if(flag == IO_WORK && (_flags & flag) == 0)
        {
            clearWork();
            addWork = true;
        }
        else
        {
            addWork = (_flags == 0);
        }
        _flags = _flags | flag;
        if(addWork)
        {
            addToWorkList();
        }
    }


    private void clearTransportFlag(int ioWork)
    {
        _flags = _flags & (~IO_WORK);
        if(_flags == 0)
        {
            clearTransportWork();
        }
    }

    void clearTransportWork()
    {
        getLink().getConnectionImpl().removeTransportWork(this);
        if(_transportWorkPrev != null)
        {
            _transportWorkPrev.setTransportWorkNext(_transportWorkNext);
        }
        if(_transportWorkNext != null)
        {
            _transportWorkNext.setTransportWorkPrev(_transportWorkPrev);

        }
        _transportWorkNext = null;
        _transportWorkPrev = null;
    }

    void addToTransportWorkList()
    {
        getLink().getConnectionImpl().addTransportWork(this);
    }


    private void setTransportFlag(int flag)
    {
        boolean addWork = (_transportFlags == 0);
        _transportFlags = _transportFlags | flag;
        if(addWork)
        {
            addToTransportWorkList();
        }
    }

    DeliveryImpl getTransportWorkNext()
    {
        return _transportWorkNext;
    }


    DeliveryImpl getTransportWorkPrev()
    {
        return _transportWorkPrev;
    }

    void setTransportWorkNext(DeliveryImpl transportWorkNext)
    {
        _transportWorkNext = transportWorkNext;
    }

    void setTransportWorkPrev(DeliveryImpl transportWorkPrev)
    {
        _transportWorkPrev = transportWorkPrev;
    }

    boolean isLocalStateChange()
    {
        return (_transportFlags & DELIVERY_STATE_CHANGED) != 0;
    }

    TransportDelivery getTransportDelivery()
    {
        return _transportDelivery;
    }

    void setTransportDelivery(TransportDelivery transportDelivery)
    {
        _transportDelivery = transportDelivery;
    }

    public boolean isSettled()
    {
        return _settled;
    }
}