/******************************************************************************
*                                                                             *
*   File Name   : msg_handler_of13.c           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-13           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "msg_handler.h"
#include "conn-svr.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "gn_inet.h"
#include "../flow-mgr/flow-mgr.h"
#include "../cluster-mgr/cluster-mgr.h"
#include "../forward-mgr/forward-mgr.h"
#include "../stats-mgr/stats-mgr.h"

convertter_t of13_convertter;
msg_handler_t of13_message_handler[OFP13_MAX_MSG];


static gn_port_t *of13_port_convertter(UINT1 *of_port, gn_port_t *new_sw_port)
{
    const struct ofp13_port *ofp = (struct ofp13_port *)of_port;
    memset(new_sw_port, 0, sizeof(gn_port_t));

    new_sw_port->port_no = ntohl(ofp->port_no);
    new_sw_port->curr = ntohl(ofp->curr);
    new_sw_port->advertised = ntohl(ofp->advertised);
    new_sw_port->supported = ntohl(ofp->supported);
    new_sw_port->peer = ntohl(ofp->peer);
    new_sw_port->config = ntohl(ofp->config);
    new_sw_port->state = ntohl(ofp->state);
    memcpy(new_sw_port->name, ofp->name, OFP_MAX_PORT_NAME_LEN);
    memcpy(new_sw_port->hw_addr, ofp->hw_addr, OFP_ETH_ALEN);

    return new_sw_port;
}

static UINT1 of13_oxm_convertter(UINT1 *oxm, gn_oxm_t *gn_oxm)
{
    struct ofp_oxm_header *of_oxm = (struct ofp_oxm_header *)oxm;
    switch(of_oxm->oxm_field_hm >> 1)
    {
        case (OFPXMT_OFB_IN_PORT):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IN_PORT);
            gn_oxm->in_port = ntohl(*(UINT4 *)(of_oxm->data));
            break;
        }
        case (OFPXMT_OFB_IN_PHY_PORT):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IN_PHY_PORT);
            gn_oxm->in_phy_port = ntohl(*(UINT4 *)(of_oxm->data));
            break;
        }
        case (OFPXMT_OFB_METADATA):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_METADATA);
            gn_oxm->metadata = gn_ntohll(*(UINT8 *)(of_oxm->data));
            break;
        }
        case (OFPXMT_OFB_ETH_DST):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_ETH_DST);
            memcpy(gn_oxm->eth_dst, of_oxm->data, OFP_ETH_ALEN);
            break;
        }
        case (OFPXMT_OFB_ETH_SRC):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_ETH_SRC);
            memcpy(gn_oxm->eth_src, of_oxm->data, OFP_ETH_ALEN);
            break;
        }
        case (OFPXMT_OFB_ETH_TYPE):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_ETH_TYPE);
            gn_oxm->eth_type = ntohs(*(UINT2 *)(of_oxm->data));
            break;
        }
        case (OFPXMT_OFB_VLAN_VID):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_VLAN_VID);
            gn_oxm->vlan_vid = ntohs(*(UINT2 *)(of_oxm->data));
            break;
        }
        case (OFPXMT_OFB_VLAN_PCP):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_VLAN_PCP);
            gn_oxm->vlan_pcp = ntohs(*(UINT2 *)(of_oxm->data));
            break;
        }
        case (OFPXMT_OFB_IP_DSCP):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IP_DSCP);
            gn_oxm->ip_dscp = *(UINT1 *)(of_oxm->data);
            break;
        }
        case (OFPXMT_OFB_IP_ECN):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IP_ECN);
            gn_oxm->ip_ecn = *(UINT1 *)(of_oxm->data);
            break;
        }
        case (OFPXMT_OFB_IP_PROTO):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IP_PROTO);
            gn_oxm->ip_proto = *(UINT1 *)(of_oxm->data);
            break;
        }
        case (OFPXMT_OFB_IPV4_SRC):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IPV4_SRC);
            gn_oxm->ipv4_src = ntohl(*(UINT4 *)(of_oxm->data));

            if(of_oxm->length > 4)
            {
                UINT4 netmask = ntohl(*(UINT4 *)(of_oxm->data + 4));
                gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IPV4_SRC_PREFIX);
                if(netmask == 0xff000000)
                {
                    gn_oxm->ipv4_src_prefix = 8;
                }
                else if(netmask == 0xffff0000)
                {
                    gn_oxm->ipv4_src_prefix = 16;
                }
                else if(netmask == 0xffffff00)
                {
                    gn_oxm->ipv4_src_prefix = 24;
                }
            }

            break;
        }
        case (OFPXMT_OFB_IPV4_DST):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IPV4_DST);
            gn_oxm->ipv4_dst = ntohl(*(UINT4 *)(of_oxm->data));

            if(of_oxm->length > 4)
            {
                UINT4 netmask = ntohl(*(UINT4 *)(of_oxm->data + 4));
                gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IPV4_DST_PREFIX);
                if(netmask == 0xff000000)
                {
                    gn_oxm->ipv4_dst_prefix = 8;
                }
                else if(netmask == 0xffff0000)
                {
                    gn_oxm->ipv4_dst_prefix = 16;
                }
                else if(netmask == 0xffffff00)
                {
                    gn_oxm->ipv4_dst_prefix = 24;
                }
            }

            break;
        }
        case (OFPXMT_OFB_TCP_SRC):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_TCP_SRC);
            gn_oxm->tcp_src = ntohs(*(UINT2 *)(of_oxm->data));
            break;
        }
        case (OFPXMT_OFB_TCP_DST):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_TCP_DST);
            gn_oxm->tcp_dst = ntohs(*(UINT2 *)(of_oxm->data));
            break;
        }
        case (OFPXMT_OFB_UDP_SRC):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_UDP_SRC);
            gn_oxm->udp_src = ntohs(*(UINT2 *)(of_oxm->data));
            break;
        }
        case (OFPXMT_OFB_UDP_DST):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_UDP_DST);
            gn_oxm->udp_dst = ntohs(*(UINT2 *)(of_oxm->data));
            break;
        }
        case (OFPXMT_OFB_SCTP_SRC):
        {
            //
            break;
        }
        case (OFPXMT_OFB_SCTP_DST):
        {
            //
            break;
        }
        case (OFPXMT_OFB_ICMPV4_TYPE):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_ICMPV4_TYPE);
            gn_oxm->icmpv4_type = *(UINT1 *)(of_oxm->data);
            break;
        }
        case (OFPXMT_OFB_ICMPV4_CODE):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_ICMPV4_CODE);
            gn_oxm->icmpv4_code = *(UINT1 *)(of_oxm->data);
            break;
        }
        case (OFPXMT_OFB_ARP_OP):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_ARP_OP);
            gn_oxm->arp_op = *(UINT1 *)(of_oxm->data);
            break;
        }
        case (OFPXMT_OFB_ARP_SPA):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_ARP_SPA);
            gn_oxm->arp_spa = ntohl(*(UINT4 *)(of_oxm->data));
            break;
        }
        case (OFPXMT_OFB_ARP_TPA):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_ARP_TPA);
            gn_oxm->arp_tpa = ntohl(*(UINT4 *)(of_oxm->data));
            break;
        }
        case (OFPXMT_OFB_ARP_SHA):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_ARP_SHA);
            memcpy(gn_oxm->arp_sha, of_oxm->data, OFP_ETH_ALEN);
            break;
        }
        case (OFPXMT_OFB_ARP_THA):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_ARP_THA);
            memcpy(gn_oxm->arp_tha, of_oxm->data, OFP_ETH_ALEN);
            break;
        }
        case (OFPXMT_OFB_IPV6_SRC):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IPV6_SRC);
            memcpy(gn_oxm->ipv6_src, of_oxm->data, OFPXMT_OFB_IPV6_SZ);

            if(of_oxm->length > OFPXMT_OFB_IPV6_SZ)
            {
                UINT4 mask[4] = {0};
                mask[0] = ntohl(*(UINT4 *)(of_oxm->data + 4));
                mask[1] = ntohl(*(UINT4 *)(of_oxm->data + 4 + 4));
                mask[2] = ntohl(*(UINT4 *)(of_oxm->data + 4 + 4 + 4));
                mask[3] = ntohl(*(UINT4 *)(of_oxm->data + 4 + 4 + 4 + 4));
                gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IPV6_SRC_PREFIX);

                //todo
            }
            break;
        }
        case (OFPXMT_OFB_IPV6_DST):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IPV6_DST);
            memcpy(gn_oxm->ipv6_dst, of_oxm->data, OFPXMT_OFB_IPV6_SZ);

            if(of_oxm->length > OFPXMT_OFB_IPV6_SZ)
            {
                UINT4 mask[4] = {0};
                mask[0] = ntohl(*(UINT4 *)(of_oxm->data + 4));
                mask[1] = ntohl(*(UINT4 *)(of_oxm->data + 4 + 4));
                mask[2] = ntohl(*(UINT4 *)(of_oxm->data + 4 + 4 + 4));
                mask[3] = ntohl(*(UINT4 *)(of_oxm->data + 4 + 4 + 4 + 4));
                gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IPV6_DST_PREFIX);

                //todo
            }
            break;
        }
        case (OFPXMT_OFB_IPV6_FLABEL):
        {
            break;
        }
        case (OFPXMT_OFB_ICMPV6_TYPE):
        {
            break;
        }
        case (OFPXMT_OFB_ICMPV6_CODE):
        {
            break;
        }
        case (OFPXMT_OFB_IPV6_ND_TARGET):
        {
            break;
        }
        case (OFPXMT_OFB_IPV6_ND_SLL):
        {
            break;
        }
        case (OFPXMT_OFB_IPV6_ND_TLL):
        {
            break;
        }
        case (OFPXMT_OFB_MPLS_LABEL):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_MPLS_LABEL);
            gn_oxm->mpls_label = ntohl(*(UINT4 *)of_oxm->data);
            break;
        }
        case (OFPXMT_OFB_MPLS_TC):
        {
            break;
        }
        case (OFPXMT_OFP_MPLS_BOS):
        {
            break;
        }
        case (OFPXMT_OFB_PBB_ISID):
        {
            break;
        }
        case (OFPXMT_OFB_TUNNEL_ID):
        {
            break;
        }
        case (OFPXMT_OFB_IPV6_EXTHDR):
        {
            break;
        }
        default:break;
    }

    return of_oxm->length;
}

convertter_t of13_convertter =
{
    .port_convertter = of13_port_convertter,
    .oxm_convertter = of13_oxm_convertter
};

static INT4 of13_msg_hello(gn_switch_t *sw, UINT1 *of_msg)
{
    UINT2 total_len = sizeof(struct ofp_hello);
    init_sendbuff(sw, OFP13_VERSION, OFPT13_HELLO, total_len, 0);
    send_of_msg(sw, total_len);

    sw->msg_driver.msg_handler[OFPT13_FEATURES_REQUEST](sw, of_msg);

    return GN_OK;
}

static INT4 of13_msg_error(gn_switch_t *sw, UINT1 *of_msg)
{
    struct ofp_error_msg *ofp_err = (struct ofp_error_msg *)of_msg;

    LOG_PROC("ERROR", "%s: switch %s 0x%llx sent error type %hu code %hu  ", FN,
            inet_htoa(ntohl(sw->sw_ip)), sw->dpid, ntohs(ofp_err->type),
            ntohs(ofp_err->code));

    switch (ntohs(ofp_err->type))
    {
    case OFPET13_HELLO_FAILED:
        printf("Hello failed");
        break;

    case OFPET13_BAD_REQUEST:
        printf("Bad request");
        break;

    case OFPET13_BAD_ACTION:
        printf("Bad action");
        break;

    case OFPET13_BAD_INSTRUCTION:
        printf("Bad instruction");
        break;

    case OFPET13_BAD_MATCH:
        printf("Bad match");
        break;

    case OFPET13_FLOW_MOD_FAILED:
        printf("Flow mod failed");
        break;

    case OFPET13_GROUP_MOD_FAILED:
        printf("Group mod failed");
        break;

    case OFPET13_PORT_MOD_FAILED:
        printf("Port mod failed");
        break;

    case OFPET13_TABLE_MOD_FAILED:
        printf("Table mod failed");
        break;

    case OFPET13_QUEUE_OP_FAILED:
        printf("Queue option failed");
        break;

    case OFPET13_SWITCH_CONFIG_FAILED:
        printf("Switch config failed");
        break;

    case OFPET13_ROLE_REQUEST_FAILED:
        printf("Role request failed");
        break;

    case OFPET13_METER_MOD_FAILED:
        printf("Meter mod failed");
        break;

    case OFPET13_TABLE_FEATURES_FAILED:
        printf("Table features failed");
        break;

    case OFPET13_EXPERIMENTER:
        printf("experimenter failed");
        break;

    default:
        break;
    }

    return GN_OK;
}

static INT4 of13_msg_echo_request(gn_switch_t *sw, UINT1 *echo_req)
{
    return sw->msg_driver.msg_handler[OFPT13_ECHO_REPLY](sw, echo_req);
    return GN_OK;
}

static INT4 of13_msg_echo_reply(gn_switch_t *sw, UINT1 *of_msg)
{
    UINT2 total_len = sizeof(struct ofp_header);
    init_sendbuff(sw, OFP13_VERSION, OFPT13_ECHO_REPLY, total_len, 0);

    return send_of_msg(sw, total_len);
}

static INT4 of13_msg_experimenter(gn_switch_t *sw, UINT1 *of_msg)
{
    //todo
    return GN_OK;
}

static INT4 of13_msg_features_request(gn_switch_t *sw, UINT1 *fea_req)
{
    UINT2 total_len = sizeof(struct ofp_header);
    init_sendbuff(sw, OFP13_VERSION, OFPT13_FEATURES_REQUEST, total_len, 0);

    return send_of_msg(sw, total_len);
}

static INT4 of13_table_miss(gn_switch_t *sw, unsigned char *of_msg)
{
    UINT2 total_len  = sizeof(struct ofp13_flow_mod) + ALIGN_8(sizeof(struct ofp_instruction)) + sizeof(struct ofp13_action_output);
    UINT1 *data = init_sendbuff(sw, OFP13_VERSION, OFPT13_FLOW_MOD, total_len, 0);

    struct ofp13_flow_mod *ofm = (struct ofp13_flow_mod *)data;
    ofm->cookie = 0x0;
    ofm->cookie_mask = 0x0;
    ofm->table_id = 0x0;
    ofm->command = OFPFC_ADD;
    ofm->idle_timeout = htons(0x0);
    ofm->hard_timeout = htons(0x0);
    ofm->priority = htons(0x0);
    ofm->buffer_id = htonl(0xffffffff);
    ofm->out_port = htonl(0xffffffff);
    ofm->out_group = htonl(0xffffffff);
    ofm->flags = htons(0x0);
    ofm->pad[0] = 0x0;
    ofm->pad[1] = 0x0;

    ofm->match.type = htons(OFPMT_OXM);
    ofm->match.length = htons(4);

    struct ofp_instruction *oin = (struct ofp_instruction *)(ofm->match.oxm_fields + 4);
    oin->type = htons(OFPIT_APPLY_ACTIONS);
    oin->len  = ntohs(0x18);

    struct ofp13_action_output *ofp13_ao = (struct ofp13_action_output *)((UINT1 *)oin + ALIGN_8(sizeof(struct ofp_instruction)));
    ofp13_ao->type    = htons(OFPAT13_OUTPUT);
    ofp13_ao->len     = htons(16);
    ofp13_ao->port    = htonl(OFPP13_CONTROLLER);
    ofp13_ao->max_len = htons(0xffff);

    return send_of_msg(sw, total_len);
}

INT4 of13_remove_all_flow(gn_switch_t *sw, UINT1 *ofmsg)
{
    UINT2 total_len = sizeof(struct ofp13_flow_mod) + ALIGN_8(sizeof(struct ofp_instruction));
    UINT1 *data = init_sendbuff(sw, OFP13_VERSION, OFPT13_FLOW_MOD, total_len, 0);

    struct ofp13_flow_mod *ofm = (struct ofp13_flow_mod *) data;
    ofm->cookie = 0x0;
    ofm->cookie_mask = 0x0;
    ofm->table_id = 0x0;
    ofm->command = OFPFC_DELETE;
    ofm->idle_timeout = htons(0x0);
    ofm->hard_timeout = htons(0x0);
    ofm->priority = htons(0x0);
    ofm->buffer_id = htonl(OFP_NO_BUFFER);
    ofm->out_port = htonl(0xffffffff);
    ofm->out_group = htonl(0xffffffff);
    ofm->flags = htons(0x0);
    ofm->pad[0] = 0x0;
    ofm->pad[1] = 0x0;

    ofm->match.type = htons(OFPMT_OXM);
    ofm->match.length = htons(4);

    struct ofp_instruction *oin =
            (struct ofp_instruction *) (ofm->match.oxm_fields + 4);
    oin->type = htons(OFPIT_APPLY_ACTIONS);
    oin->len = htons(ALIGN_8(sizeof(struct ofp_instruction)));

    return send_of_msg(sw, total_len);
}

static INT4 of13_msg_features_reply(gn_switch_t *sw, UINT1 *of_msg)
{
    struct ofp13_switch_features *osf = (struct ofp13_switch_features *)of_msg;
    stats_req_info_t stats_req_info;

    sw->dpid = gn_ntohll(osf->datapath_id);
    sw->n_buffers = ntohl(osf->n_buffers);
    sw->n_tables = osf->n_tables;
    sw->capabilities = ntohl(osf->capabilities);

    sw->msg_driver.msg_handler[OFPT13_SET_CONFIG](sw, of_msg);
    sw->msg_driver.msg_handler[OFPT13_GET_CONFIG_REQUEST](sw, of_msg);

    stats_req_info.flags = 0;
    stats_req_info.type = OFPMP_DESC;
    sw->msg_driver.msg_handler[OFPT13_MULTIPART_REQUEST](sw, (UINT1 *)&stats_req_info);

    stats_req_info.type = OFPMP_PORT_DESC;
    sw->msg_driver.msg_handler[OFPT13_MULTIPART_REQUEST](sw, (UINT1 *)&stats_req_info);

    stats_req_info.type = OFPMP_TABLE;
    sw->msg_driver.msg_handler[OFPT13_MULTIPART_REQUEST](sw, (UINT1 *)&stats_req_info);

    stats_req_info.type = OFPMP_METER_FEATURES;
    sw->msg_driver.msg_handler[OFPT13_MULTIPART_REQUEST](sw, (UINT1 *)&stats_req_info);

//    stats_req_info.type = OFPMP_FLOW;
//    sw->msg_driver.msg_handler[OFPT13_MULTIPART_REQUEST](sw, (UINT1 *)&stats_req_info);

    {
        UINT1 dpid[8];
        ulli64_to_uc8(sw->dpid, dpid);
        LOG_PROC("INFO", "New Openflow13 switch [%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x] connected: ip[%s:%d]", dpid[0],
                dpid[1], dpid[2], dpid[3], dpid[4], dpid[5], dpid[6], dpid[7], inet_htoa(ntohl(sw->sw_ip)), ntohs(sw->sw_port));
    }
    return GN_OK;
}

static INT4 of13_msg_get_config_request(gn_switch_t *sw, UINT1 *get_conf_req)
{
    UINT2 total_len = sizeof(struct ofp_header);
    init_sendbuff(sw, OFP13_VERSION, OFPT13_GET_CONFIG_REQUEST, total_len, 0);

    return send_of_msg(sw, total_len);
}

static INT4 of13_msg_get_config_reply(gn_switch_t *sw, UINT1 *of_msg)
{
    sw->msg_driver.msg_handler[OFPT13_BARRIER_REQUEST](sw, of_msg);
    return GN_OK;
}

static INT4 of13_msg_set_config(gn_switch_t *sw, UINT1 *config)
{
    UINT2 total_len = sizeof(struct ofp13_switch_config);
    UINT1 * data = init_sendbuff(sw, OFP13_VERSION, OFPT13_SET_CONFIG, total_len, 0);

    struct ofp13_switch_config *of_config = (struct ofp13_switch_config *)data;
//    of_config->flags = htons(OFPC_FRAG_MASK);
    of_config->flags = htons(OFPC_FRAG_NORMAL);
    of_config->miss_send_len = htons(1500);

    return send_of_msg(sw, total_len);
}

UINT4 of13_get_pkt_inport(struct ofp13_packet_in *of_pkt_in)
{
    UINT4 inport = 0;
    UINT4 len = ntohs(of_pkt_in->match.length);
    struct ofp_oxm_header *oxm_ptr = (void *) (of_pkt_in->match.oxm_fields);

    if (ntohs(of_pkt_in->match.type) != OFPMT_OXM || len < sizeof(struct ofpx_match))
    {
        return GN_ERR;
    }

    if (ntohs(oxm_ptr->oxm_class) != OFPXMC_OPENFLOW_BASIC)
    {
        return GN_ERR;
    }

    if (oxm_ptr->oxm_field_hm == OFPXMT_OFB_IN_PORT)
    {
        inport = *(UINT4 *) (oxm_ptr->data);
    }

    return ntohl(inport);
}

UINT1 *of13_get_pkt_data(struct ofp13_packet_in *of_pkt_in)
{
    UINT1 *data = NULL;
    UINT4 match_len = 0;
    UINT4 pkt_ofs = 0;

    match_len = ALIGN_8(htons(of_pkt_in->match.length));
    match_len -= sizeof(of_pkt_in->match);
    pkt_ofs = (sizeof(*of_pkt_in) + match_len + 2);
    data = (void *)((UINT1 *)(of_pkt_in) + pkt_ofs);

    return data;
}

static INT4 of13_msg_packet_in(gn_switch_t *sw, UINT1 *of_msg)
{
    packet_in_info_t packet_in_info;
    struct ofp13_packet_in *of_pkt = (struct ofp13_packet_in *)of_msg;
    struct ofp_oxm_header *oxm_ptr = (void *)(of_pkt->match.oxm_fields);

    packet_in_info.xid = ntohl(of_pkt->header.xid);
    packet_in_info.buffer_id = ntohl(of_pkt->buffer_id);
    packet_in_info.inport = ntohl(*(UINT4 *)(oxm_ptr->data));
    packet_in_info.data_len = ntohs(of_pkt->header.length) - sizeof(struct ofp13_packet_in)
                    - ALIGN_8(htons(of_pkt->match.length)) + sizeof(struct ofpx_match) - 2;

    packet_in_info.data = of13_get_pkt_data(of_pkt);

//    ether_t *ether_header = (ether_t *)packet_in_info.data;
//    if(ether_header->proto != htons(ETHER_LLDP)){
//    	printf("Table id: %d,Protocal : 0x%x \n",of_pkt->table_id,htonl(ether_header->proto));
//    }
    return packet_in_process(sw, &packet_in_info);
}

static void of13_parse_match(struct ofpx_match *of_match, gn_match_t *gn_match)
{
    UINT2 oxm_tot_len = ntohs(of_match->length);
    UINT2 oxm_len = 4;
    UINT1 field_len = 0;
    UINT1 *oxm = of_match->oxm_fields;

    memset(gn_match, 0, sizeof(gn_match_t));
    while(ALIGN_8(oxm_len) < oxm_tot_len)
    {
        field_len = of13_oxm_convertter((UINT1 *)oxm, &(gn_match->oxm_fields));
        oxm += sizeof(struct ofp_oxm_header) + field_len;
        oxm_len += sizeof(struct ofp_oxm_header) + field_len;
    }
}

static INT4 of13_msg_flow_removed(gn_switch_t *sw, UINT1 *of_msg)
{
    struct ofp13_flow_removed *ofp_fr = (struct ofp13_flow_removed *)of_msg;

    gn_flow_t flow;
    flow.priority = ntohl(ofp_fr->priority);
    flow.table_id = ofp_fr->table_id;

    memset(&flow, 0, sizeof(gn_flow_t));
    of13_parse_match(&(ofp_fr->match), &(flow.match));

    return flow_entry_timeout(sw, &flow);
}

static INT4 of13_msg_port_status(gn_switch_t *sw, UINT1 *of_msg)
{
    INT4 port;
    gn_port_t new_sw_ports;

    struct ofp13_port_status *ops = (struct ofp13_port_status *)of_msg;

    if (ops->reason == OFPPR_ADD)
    {
        LOG_PROC("INFO", "New port found: %s", ops->desc.name);
        of13_port_convertter((UINT1 *)&ops->desc, &new_sw_ports);

        //Ĭ���������1000Mbps
        new_sw_ports.stats.max_speed = 1000000;  //1073741824 = 1024^3, 1048576 = 1024^2
        sw->ports[sw->n_ports] = new_sw_ports;
        sw->n_ports++;

        return GN_OK;
    }
    else if (ops->reason == OFPPR_DELETE)
    {
        LOG_PROC("INFO", "Port deleted: %s", ops->desc.name);
    }
    else if (ops->reason == OFPPR_MODIFY)
    {
        LOG_PROC("INFO", "Port state change: %s[new state: %d]", ops->desc.name, ntohl(ops->desc.state));
    }

    //ɾ��Ŀ��ת����down��������
//    l2_del_flowentry_by_portno(sw, ntohl(ops->desc.port_no));

    //��������
    for (port = 0; port < sw->n_ports; port++)
    {
        if (sw->ports[port].port_no == ntohl(ops->desc.port_no))
        {
            sw->ports[port].state = ntohl(ops->desc.state);
            free(sw->neighbor[port]);
            sw->neighbor[port] = NULL;
            break;
        }
    }

    return GN_OK;
}

static INT4 of13_msg_packet_out(gn_switch_t *sw, UINT1 *pktout_req)
{
    packout_req_info_t *packout_req_info = (packout_req_info_t *)pktout_req;
    UINT2 total_len = sizeof(struct ofp13_packet_out) + sizeof(struct ofp13_action_output);
    if (packout_req_info->buffer_id == OFP_NO_BUFFER)
    {
        total_len += packout_req_info->data_len;
    }

    UINT1 *data = init_sendbuff(sw, OFP13_VERSION, OFPT13_PACKET_OUT, total_len, packout_req_info->xid);

    struct ofp13_packet_out *of13_out = (struct ofp13_packet_out*)data;
    of13_out->buffer_id = htonl(packout_req_info->buffer_id);
    of13_out->in_port = htonl(packout_req_info->inport);
    of13_out->actions_len = htons(sizeof(struct ofp13_action_output));

    struct ofp13_action_output *ofp13_act = (struct ofp13_action_output *)of13_out->actions;
    ofp13_act->type = htons(OFPAT13_OUTPUT);
    ofp13_act->len = htons(sizeof(struct ofp13_action_output));
    ofp13_act->port = htonl(packout_req_info->outport);    //�Ӹ����˿ڷ���ȥ
    ofp13_act->max_len = htons(packout_req_info->max_len);
    memset(ofp13_act->pad, 0x0, 6);

    if (packout_req_info->buffer_id == OFP_NO_BUFFER)
    {
        memcpy(ofp13_act->pad+6, packout_req_info->data, packout_req_info->data_len);
    }

    return send_of_msg(sw, total_len);
}

static UINT2 of13_add_oxm_field(UINT1 *buf, gn_oxm_t *oxm_fields)
{
    UINT2 oxm_len = 0;
    struct ofp_oxm_header *oxm = (void *)buf;
    size_t oxm_field_sz = 0;
    UINT4 netmask = 0;
    UINT4 netmaskv6[4] = {0};

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IN_PORT))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_IN_PORT << 1;
        oxm->length = OFPXMT_OFB_IN_PORT_SZ;
        *(UINT4 *)(oxm->data) = htonl(oxm_fields->in_port);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_IN_PORT_SZ;
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IN_PHY_PORT))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_IN_PHY_PORT << 1;
        oxm->length = OFPXMT_OFB_IN_PORT_SZ;
        *(UINT4 *)(oxm->data) = htonl(oxm_fields->in_phy_port);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_IN_PORT_SZ;
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_METADATA))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_METADATA << 1;
        oxm->length = 8;
        *(UINT8 *)(oxm->data) = htonl(oxm_fields->metadata);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + 8;
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ETH_DST))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_ETH_DST << 1;
        oxm->length = OFPXMT_OFB_ETH_SZ;
        memcpy((UINT1 *)(oxm->data), oxm_fields->eth_dst, OFP_ETH_ALEN);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_ETH_SZ;
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ETH_SRC))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_ETH_SRC << 1;
        oxm->length = OFPXMT_OFB_ETH_SZ;
        memcpy((UINT1 *)(oxm->data), oxm_fields->eth_src, OFP_ETH_ALEN);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_ETH_SZ;
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ETH_TYPE))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_ETH_TYPE << 1;
        oxm->length = OFPXMT_OFB_ETH_TYPE_SZ;
        *(UINT2 *)(oxm->data) = htons(oxm_fields->eth_type);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_ETH_TYPE_SZ;
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_VLAN_VID))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_VLAN_VID << 1;
        oxm->length = OFPXMT_OFB_VLAN_VID_SZ;
        oxm_fields->vlan_vid = OFPVID_PRESENT | oxm_fields->vlan_vid;
        *(UINT2 *)(oxm->data) = htons(oxm_fields->vlan_vid);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_VLAN_VID_SZ;
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_VLAN_PCP))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_VLAN_PCP << 1;
        oxm->length = OFPXMT_OFB_VLAN_PCP_SZ;
        *(UINT1 *)(oxm->data) = htons(oxm_fields->vlan_pcp);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_VLAN_PCP_SZ;
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IP_DSCP))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_IP_DSCP << 1;
        oxm->length = OFPXMT_OFB_IP_DSCP_SZ;
        *(UINT1 *)(oxm->data) = oxm_fields->ip_dscp;

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_IP_DSCP_SZ;
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IP_ECN))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_IP_ECN << 1;
        oxm->length = OFPXMT_OFB_IP_DSCP_SZ;
        *(UINT1 *)(oxm->data) = oxm_fields->ip_ecn;

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_IP_DSCP_SZ;
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IP_PROTO))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_IP_PROTO << 1;
        oxm->length = OFPXMT_OFB_IP_PROTO_SZ;
        *(UINT1 *)(oxm->data) = oxm_fields->ip_proto;

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_IP_PROTO_SZ;
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV4_SRC))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);

        if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV4_SRC_PREFIX))  //������
        {
            oxm->oxm_field_hm = (OFPXMT_OFB_IPV4_SRC << 1) + 1 ;
            oxm->length = OFPXMT_OFB_IPV4_SZ * 2;
            *(UINT4 *)(oxm->data) = htonl(oxm_fields->ipv4_src);
            switch(oxm_fields->ipv4_src_prefix)
            {
                case 8:
                {
                    netmask = 0xff000000;
                    break;
                }
                case 16:
                {
                    netmask = 0xffff0000;
                    break;
                }
                case 24:
                {
                    netmask = 0xffffff00;
                    break;
                }
                default:break;
            }

            *(UINT4 *)(oxm->data + 4) = htonl(netmask);
            oxm_len += OFPXMT_OFB_IPV4_SZ;
        }
        else    //������
        {
            oxm->oxm_field_hm = OFPXMT_OFB_IPV4_SRC << 1;
            oxm->length = OFPXMT_OFB_IPV4_SZ;
            *(UINT4 *)(oxm->data) = htonl(oxm_fields->ipv4_src);
        }

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_IPV4_SZ;
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV4_DST))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);

        if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV4_DST_PREFIX))  //������
        {
            oxm->oxm_field_hm = (OFPXMT_OFB_IPV4_DST << 1) + 1 ;
            oxm->length = OFPXMT_OFB_IPV4_SZ * 2;
            *(UINT4 *)(oxm->data) = htonl(oxm_fields->ipv4_dst);
            switch(oxm_fields->ipv4_dst_prefix)
            {
                case 8:
                {
                    netmask = 0xff000000;
                    break;
                }
                case 16:
                {
                    netmask = 0xffff0000;
                    break;
                }
                case 24:
                {
                    netmask = 0xffffff00;
                    break;
                }
                default:break;
            }

            *(UINT4 *)(oxm->data + 4) = htonl(netmask);
            oxm_len += OFPXMT_OFB_IPV4_SZ;
        }
        else  //������
        {
            oxm->oxm_field_hm = OFPXMT_OFB_IPV4_DST << 1;
            oxm->length = OFPXMT_OFB_IPV4_SZ;
            *(UINT4 *)(oxm->data) = htonl(oxm_fields->ipv4_dst);
        }

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_IPV4_SZ;
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_TCP_SRC))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_TCP_SRC << 1;
        oxm->length = OFPXMT_OFB_L4_PORT_SZ;
        *(UINT2 *)(oxm->data) = htons(oxm_fields->tcp_src);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_L4_PORT_SZ;
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_TCP_DST))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_TCP_DST << 1;
        oxm->length = OFPXMT_OFB_L4_PORT_SZ;
        *(UINT2 *)(oxm->data) = htons(oxm_fields->tcp_dst);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_L4_PORT_SZ;
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_UDP_SRC))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_UDP_SRC << 1;
        oxm->length = OFPXMT_OFB_L4_PORT_SZ;
        *(UINT2 *)(oxm->data) = htons(oxm_fields->udp_src);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_L4_PORT_SZ;
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_UDP_DST))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_UDP_DST << 1;
        oxm->length = OFPXMT_OFB_L4_PORT_SZ;
        *(UINT2 *)(oxm->data) = htons(oxm_fields->udp_dst);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_L4_PORT_SZ;
    }

//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_SCTP_SRC))
//    {
//
//    }
//
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_SCTP_DST))
//    {
//
//    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ICMPV4_TYPE))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_ICMPV4_TYPE << 1;
        oxm->length = 1;
        *(UINT1 *)(oxm->data) = oxm_fields->icmpv4_type;

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + 1;
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ICMPV4_CODE))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_ICMPV4_CODE << 1;
        oxm->length = 1;
        *(UINT1 *)(oxm->data) = oxm_fields->icmpv4_code;

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + 1;
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ARP_OP))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_ARP_OP << 1;
        oxm->length = 1;
        *(UINT1 *)(oxm->data) = oxm_fields->arp_op;

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + 1;
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ARP_SPA))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_ARP_SPA << 1;
        oxm->length = OFPXMT_OFB_IPV4_SZ;
        *(UINT4 *)(oxm->data) = htonl(oxm_fields->arp_spa);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_IPV4_SZ;
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ARP_TPA))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_ARP_TPA << 1;
        oxm->length = OFPXMT_OFB_IPV4_SZ;
        *(UINT4 *)(oxm->data) = htonl(oxm_fields->arp_tpa);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_IPV4_SZ;
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ARP_SHA))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_ARP_SHA << 1;
        oxm->length = OFPXMT_OFB_ETH_SZ;
        memcpy((UINT1 *)(oxm->data), oxm_fields->arp_sha, OFP_ETH_ALEN);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_ETH_TYPE_SZ;
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ARP_THA))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_ARP_THA << 1;
        oxm->length = OFPXMT_OFB_ETH_SZ;
        memcpy((UINT1 *)(oxm->data), oxm_fields->arp_tha, OFP_ETH_ALEN);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_ETH_TYPE_SZ;
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV6_SRC))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);

        if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV6_SRC_PREFIX))
        {
            oxm->oxm_field_hm = (OFPXMT_OFB_IPV6_SRC << 1) + 1 ;
            oxm->length = OFPXMT_OFB_IPV6_SZ + OFPXMT_OFB_IPV6_SZ;
            memcpy((UINT1 *)(oxm->data), oxm_fields->ipv6_src, OFPXMT_OFB_IPV6_SZ);

            switch(oxm_fields->ipv6_src_prefix)
            {
                case 8:
                {
                    netmaskv6[0] = htonl(0xff000000);
                    netmaskv6[1] = 0x0;
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 16:
                {
                    netmaskv6[0] = htonl(0xffff0000);
                    netmaskv6[1] = 0x0;
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 24:
                {
                    netmaskv6[0] = htonl(0xffffff00);
                    netmaskv6[1] = 0x0;
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 32:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = 0x0;
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 40:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xff000000);
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 48:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffff0000);
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 56:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffff00);
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 64:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 72:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xff000000);
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 80:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffff0000);
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 88:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffff00);
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 96:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffffff);
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 104:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffffff);
                    netmaskv6[3] = htonl(0xff000000);
                    break;
                }
                case 112:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffffff);
                    netmaskv6[3] = htonl(0xffff0000);
                    break;
                }
                case 120:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffffff);
                    netmaskv6[3] = htonl(0xffffff00);
                    break;
                }
                case 128:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffffff);
                    netmaskv6[3] = htonl(0xffffffff);
                    break;
                }
                default:break;
            }

            memcpy((UINT1 *)(oxm->data + OFPXMT_OFB_IPV6_SZ), netmaskv6, OFPXMT_OFB_IPV6_SZ);
            oxm_len += OFPXMT_OFB_IPV6_SZ;
        }
        else   //������
        {
            oxm->oxm_field_hm = OFPXMT_OFB_IPV6_SRC << 1;
            oxm->length = OFPXMT_OFB_IPV6_SZ;
            memcpy((UINT1 *)(oxm->data), oxm_fields->ipv6_src, 16);
        }

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_IPV6_SZ;
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV6_DST))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);

        if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV6_DST_PREFIX))
        {
            oxm->oxm_field_hm = (OFPXMT_OFB_IPV6_DST << 1) + 1 ;
            oxm->length = OFPXMT_OFB_IPV6_SZ + OFPXMT_OFB_IPV6_SZ;
            memcpy((UINT1 *)(oxm->data), oxm_fields->ipv6_dst, OFPXMT_OFB_IPV6_SZ);

            switch(oxm_fields->ipv6_dst_prefix)
            {
                case 8:
                {
                    netmaskv6[0] = htonl(0xff000000);
                    netmaskv6[1] = 0x0;
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 16:
                {
                    netmaskv6[0] = htonl(0xffff0000);
                    netmaskv6[1] = 0x0;
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 24:
                {
                    netmaskv6[0] = htonl(0xffffff00);
                    netmaskv6[1] = 0x0;
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 32:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = 0x0;
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 40:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xff000000);
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 48:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffff0000);
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 56:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffff00);
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 64:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 72:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xff000000);
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 80:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffff0000);
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 88:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffff00);
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 96:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffffff);
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 104:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffffff);
                    netmaskv6[3] = htonl(0xff000000);
                    break;
                }
                case 112:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffffff);
                    netmaskv6[3] = htonl(0xffff0000);
                    break;
                }
                case 120:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffffff);
                    netmaskv6[3] = htonl(0xffffff00);
                    break;
                }
                case 128:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffffff);
                    netmaskv6[3] = htonl(0xffffffff);
                    break;
                }
            }
            memcpy((UINT1 *)(oxm->data + OFPXMT_OFB_IPV6_SZ), netmaskv6, OFPXMT_OFB_IPV6_SZ);
            oxm_len += OFPXMT_OFB_IPV6_SZ;
        }
        else  //������
        {
            oxm->oxm_field_hm = OFPXMT_OFB_IPV6_DST << 1;
            oxm->length = OFPXMT_OFB_IPV6_SZ;
            memcpy((UINT1 *)(oxm->data), oxm_fields->ipv6_dst, 16);
        }

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_IPV6_SZ;
    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_IPV6_FLABEL))
//    {
//    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_ICMPV6_TYPE))
//    {
//    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_ICMPV6_CODE))
//    {
//    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_IPV6_ND_TARGET))
//    {
//    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_IPV6_ND_SLL))
//    {
//    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_IPV6_ND_TLL))
//    {
//    }
    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_MPLS_LABEL))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_MPLS_LABEL << 1;
        oxm->length = 4;
        *(UINT4 *)(oxm->data) = htonl(oxm_fields->mpls_label);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + 4;
    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_MPLS_TC))
//    {
//    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFP_MPLS_BOS))
//    {
//    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_PBB_ISID))
//    {
//    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_TUNNEL_ID))
//    {
//    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_IPV6_EXTHDR))
//    {
//    }

    return oxm_len;
}

UINT2 of13_add_match(struct ofpx_match *match, gn_match_t *gn_match)
{
    UINT2 oxm_len = 0;
    UINT2 match_len = 0;

    oxm_len = of13_add_oxm_field(match->oxm_fields, &(gn_match->oxm_fields));
    match_len = sizeof(struct ofpx_match) - 4 + oxm_len;
    return match_len;
}

UINT2 of13_add_action(UINT1 *buf, gn_action_t *action)
{
    UINT2 action_len = 0;
    UINT1 *act = buf;
    gn_action_t *p_action = action;

    while(p_action)
    {
        switch(p_action->type)
        {
            case OFPAT13_OUTPUT:
            {
                gn_action_output_t *p_action_output = (gn_action_output_t *)p_action;
                struct ofp13_action_output *ofp13_ao = (struct ofp13_action_output *)act;
                ofp13_ao->type = htons(OFPAT13_OUTPUT);
                ofp13_ao->len = htons(16);
                ofp13_ao->port = htonl(p_action_output->port);
                ofp13_ao->max_len = htons(p_action_output->max_len);
                memset(ofp13_ao->pad, 0x0, 6);

                act += 16;
                action_len += 16;
                break;
            }
            case OFPAT13_COPY_TTL_OUT:
            {
                break;
            }
            case OFPAT13_COPY_TTL_IN:
            {
                break;
            }
            case OFPAT13_MPLS_TTL:
            {
                break;
            }
            case OFPAT13_DEC_MPLS_TTL:
            {
                break;
            }
            case OFPAT13_PUSH_VLAN:
            {
//                printf("action push vlan\n");
                struct ofp_action_push *oa_push = (struct ofp_action_push *)act;
                oa_push->type = htons(OFPAT13_PUSH_VLAN);
                oa_push->len = htons(sizeof(struct ofp_action_push));
                oa_push->ethertype = htons(ETH_TYPE_VLAN_8021Q);
                memset(oa_push->pad, 0x0, 2);
                act += 8;
                action_len += 8;
                break;
            }
            case OFPAT13_POP_VLAN:
            {
//                printf("action pop vlan\n");
                struct ofp_action_pop_mpls *oa_pop = (struct ofp_action_pop_mpls *)act;
                oa_pop->type = htons(OFPAT13_POP_VLAN);
                oa_pop->len = htons(sizeof(struct ofp_action_pop_mpls));
                oa_pop->ethertype = htons(ETH_TYPE_VLAN_8021Q);
                memset(oa_pop->pad, 0x0, 2);
                act += 8;
                action_len += 8;
                break;
            }
            case OFPAT13_PUSH_MPLS:
            {
//                printf("action push mpls\n");
                struct ofp_action_push *oa_push = (struct ofp_action_push *)act;
                oa_push->type = htons(OFPAT13_PUSH_VLAN);
                oa_push->len = htons(sizeof(struct ofp_action_push));
                oa_push->ethertype = htons(ETH_TYPE_MPLS_8021Q);
                memset(oa_push->pad, 0x0, 2);
                act += 8;
                action_len += 8;
                break;
            }
            case OFPAT13_POP_MPLS:
            {
//                printf("action pop mpls\n");
                struct ofp_action_pop_mpls *oa_pop = (struct ofp_action_pop_mpls *)act;
                oa_pop->type = htons(OFPAT13_POP_VLAN);
                oa_pop->len = htons(sizeof(struct ofp_action_pop_mpls));
                oa_pop->ethertype = htons(ETH_TYPE_MPLS_8021Q);
                memset(oa_pop->pad, 0x0, 2);
                act += 8;
                action_len += 8;
                break;
            }
            case OFPAT13_SET_QUEUE:
            {
                break;
            }
            case OFPAT13_GROUP:
            {
                gn_action_group_t *p_action_group = (gn_action_group_t *)p_action;
                struct ofp_action_group *ofp13_ag = (struct ofp_action_group *)act;
                ofp13_ag->type     = htons(OFPAT13_GROUP);
                ofp13_ag->len      = htons(sizeof(struct ofp_action_group));;
                ofp13_ag->group_id = htonl(p_action_group->group_id);

                act += 8;
                action_len += 8;
                break;
            }
            case OFPAT13_SET_NW_TTL:
            {
                break;
            }
            case OFPAT13_DEC_NW_TTL:
            {
                break;
            }
            case OFPAT13_SET_FIELD:
            {
//                printf("action set\n");
                gn_action_set_field_t *p_action_set_field = (gn_action_set_field_t *)p_action;
                UINT2 oxm_len = 0;
                struct ofp_action_set_field *oa_set = (struct ofp_action_set_field *)act;
                oa_set->type = htons(OFPAT13_SET_FIELD);

//                oxm_len = ALIGN_8(of13_add_oxm_field(oa_set->field, &(p_action_set_field->oxm_fields)));
                oxm_len = of13_add_oxm_field(oa_set->field, &(p_action_set_field->oxm_fields));
                oa_set->len = htons(ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + oxm_len));

                act = act + ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + oxm_len);
                action_len = action_len + ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + oxm_len);
                break;
            }
            case OFPAT13_PUSH_PBB:
            {
                break;
            }
            case OFPAT13_POP_PBB:
            {
                break;
            }
            case OFPAT13_EXPERIMENTER:
            {
                break;
            }
            default:break;
        }
        p_action = p_action->next;
    }

    return action_len;
}

UINT2 of13_add_instruction(UINT1 *buf, gn_flow_t *flow)
{
    UINT2 instruction_len = 0;
    UINT2 action_len = 0;
    UINT1 *ins = buf;
    gn_instruction_t *p_ins = flow->instructions;

    if(NULL == p_ins)
    {
        struct ofp_instruction_actions *oin = (struct ofp_instruction_actions *)ins;
        oin->type = ntohs(OFPIT_APPLY_ACTIONS);
        oin->pad[0] = 0;
        oin->pad[1] = 0;
        oin->pad[2] = 0;
        oin->pad[3] = 0;

        oin->len = ntohs(sizeof(struct ofp_instruction_actions));

        ins = ins + 8;
        instruction_len = instruction_len + 8;
        return instruction_len;
    }

    while(p_ins)
    {
        if((p_ins->type == OFPIT_APPLY_ACTIONS) || (p_ins->type == OFPIT_WRITE_ACTIONS))
        {
//            printf("Instruct apply/write actions\n");
            gn_instruction_actions_t *p_ins_actions = (gn_instruction_actions_t *)p_ins;
            struct ofp_instruction_actions *oin = (struct ofp_instruction_actions *)ins;
            oin->type = ntohs(p_ins->type);
            oin->pad[0] = 0;
            oin->pad[1] = 0;
            oin->pad[2] = 0;
            oin->pad[3] = 0;

            action_len = of13_add_action((UINT1 *)(oin->actions), p_ins_actions->actions);
            oin->len = ntohs(sizeof(struct ofp_instruction_actions) + ALIGN_8(action_len));
//            printf("action len: %d, instruction len: %d\n", action_len, sizeof(struct ofp_instruction_actions) + action_len);

            ins = ins + 8 + action_len;
            instruction_len = instruction_len + 8 + action_len;
        }
        else if(p_ins->type == OFPIT_CLEAR_ACTIONS)
        {
//            printf("Instruct clear actions\n");
            struct ofp_instruction_actions *oin = (struct ofp_instruction_actions *)ins;
            oin->type = ntohs(OFPIT_CLEAR_ACTIONS);
            oin->len = ntohs(8);
            oin->pad[0] = 0;
            oin->pad[1] = 0;
            oin->pad[2] = 0;
            oin->pad[3] = 0;

            ins += 8;
            instruction_len += 8;
        }
        else if(p_ins->type == OFPIT_GOTO_TABLE)
        {
//            printf("Instruct goto table\n");
            gn_instruction_goto_table_t *p_ins_goto_table = (gn_instruction_goto_table_t *)p_ins;
            struct ofp_instruction_goto_table *oi_table = (struct ofp_instruction_goto_table *)ins;
            oi_table->type = ntohs(OFPIT_GOTO_TABLE);
            oi_table->len  = ntohs(8);
            oi_table->table_id = p_ins_goto_table->table_id;
            memset(oi_table->pad, 0x0, 3);

            ins += 8;
            instruction_len += 8;
        }
        else if(p_ins->type == OFPIT_METER)
        {
//            printf("Instruct meter\n");
            gn_instruction_meter_t *p_ins_meter = (gn_instruction_meter_t *)p_ins;
            struct ofp_instruction_meter *oi_meter = (struct ofp_instruction_meter *)ins;
            oi_meter->type = htons(OFPIT_METER);
            oi_meter->len = htons(8);
            oi_meter->meter_id = htonl(p_ins_meter->meter_id);

            ins += 8;
            instruction_len += 8;
        }

        p_ins = p_ins->next;
    }

    return instruction_len;
}

static INT4 of13_msg_flow_mod(gn_switch_t *sw, UINT1 *flowmod_req)
{
    UINT2 match_len = 0;
    UINT2 instruction_len = 0;
    UINT2 tot_len = sizeof(struct ofp13_flow_mod);
    UINT1 *data = init_sendbuff(sw, OFP13_VERSION, OFPT13_FLOW_MOD, tot_len, 0);
    flow_mod_req_info_t *mod_info = (flow_mod_req_info_t *)flowmod_req;

    struct ofp13_flow_mod *ofm = (struct ofp13_flow_mod *)data;
    ofm->cookie = gn_htonll(0x0);
    ofm->cookie_mask = gn_htonll(0x0);
    ofm->table_id = mod_info->flow->table_id;
    ofm->command = mod_info->command;
    ofm->idle_timeout = htons(mod_info->flow->idle_timeout);
    ofm->hard_timeout = htons(mod_info->flow->hard_timeout);
    ofm->priority = htons(mod_info->flow->priority);
    ofm->buffer_id = htonl(mod_info->buffer_id);                //Default 0xffffffff
    ofm->out_port = htonl(mod_info->out_port);                  //Default 0xffffffff
    ofm->out_group = htonl(mod_info->out_group);                //Default 0xffffffff
    ofm->flags = htons(mod_info->flags);                        //Default OFPFF13_SEND_FLOW_REM
    ofm->pad[0] = 0x0;
    ofm->pad[1] = 0x0;

    ofm->match.type = htons(mod_info->flow->match.type);        //Default OFPMT_OXM
    match_len = of13_add_match(&ofm->match, &(mod_info->flow->match));
    ofm->match.length = ntohs(match_len);
    tot_len = tot_len - sizeof(struct ofpx_match) + ALIGN_8(match_len);

//    if(DEBUG)
//    {
//        printf("match_len: %d   %d\n", match_len, ALIGN_8(match_len));
//        printf("toto_len: %d\n", tot_len);
//    }

    instruction_len = of13_add_instruction(data + tot_len, mod_info->flow);
    tot_len += ALIGN_8(instruction_len);
    ofm->header.length = htons(tot_len);

    return send_of_msg(sw, tot_len);
}

static INT4 of13_msg_group_mod(gn_switch_t *sw, UINT1 *groupmod_req)
{
    group_mod_req_info_t *group_mod_req_info = (group_mod_req_info_t *)groupmod_req;
    UINT2 total_len = sizeof(struct ofp_group_mod);
    UINT1 *data = init_sendbuff(sw, OFP13_VERSION, OFPT13_GROUP_MOD, total_len, 0);

    struct ofp_group_mod *ofp13_group = (struct ofp_group_mod *) data;
    ofp13_group->command = htons(group_mod_req_info->command);
    ofp13_group->type = group_mod_req_info->group->type;
    ofp13_group->group_id = htonl(group_mod_req_info->group->group_id);

    UINT2 bucket_len = 0;
    UINT2 action_len = 0;
    struct ofp_bucket *ofp13_bucket = NULL;
    group_bucket_t *p_bucket = group_mod_req_info->group->buckets;
    while(p_bucket)
    {
        ofp13_bucket = (struct ofp_bucket *)((UINT1 *)ofp13_group->buckets + bucket_len);
        ofp13_bucket->weight = htons(p_bucket->weight);
        ofp13_bucket->watch_port = htonl(p_bucket->watch_port);
        ofp13_bucket->watch_group = htonl(p_bucket->watch_group);
        action_len = of13_add_action((UINT1 *)ofp13_bucket->actions, p_bucket->actions);
        ofp13_bucket->len = htons(sizeof(struct ofp_bucket) + action_len);
        bucket_len += action_len;
    }

    return send_of_msg(sw, total_len);
}

static INT4 of13_msg_port_mod(gn_switch_t *sw, UINT1 *port)
{
    UINT2 total_len = sizeof(struct ofp13_port_mod);
    UINT1 *data = init_sendbuff(sw, OFP13_VERSION, OFPT13_PORT_MOD, total_len, 0);
    struct ofp13_port_mod *opm_in = (struct ofp13_port_mod *)port;
    struct ofp13_port_mod *opm = (struct ofp13_port_mod *)data;
    opm->port_no = htonl(opm_in->port_no);
    memset(opm->pad, 0x0, 4);
    memcpy(opm->hw_addr, opm_in->hw_addr, OFP_ETH_ALEN);
    memset(opm->pad2, 0x0, 2);
    opm->config = htonl(0x0);
    opm->mask = htonl(0x0);
    opm->advertise = htonl(0x0);
    memset(opm->pad3, 0x0, 4);

    return send_of_msg(sw, total_len);
}

static INT4 of13_msg_table_mod(gn_switch_t *sw, UINT1 *of_msg)
{
    //todo
    return GN_OK;
}

static INT4 of13_msg_multipart_request(gn_switch_t *sw, UINT1 *mtp_req)
{
    stats_req_info_t *stats_req_info = (stats_req_info_t *)mtp_req;
    UINT2 total_len = sizeof(struct ofp_multipart_request);
    UINT1 *data = init_sendbuff(sw, OFP13_VERSION, OFPT13_MULTIPART_REQUEST, total_len, 0);
    struct ofp_multipart_request *ofp_mr = (struct ofp_multipart_request *)data;
    ofp_mr->type = htons(stats_req_info->type);
    ofp_mr->flags = htons(stats_req_info->flags);

    switch(stats_req_info->type)
    {
        case OFPMP_FLOW:
        {
            flow_stats_req_data_t *flow_stats_req_data = (flow_stats_req_data_t *)(stats_req_info->data);
            struct ofp13_flow_stats_request *ofp_fsr = (struct ofp13_flow_stats_request *) (ofp_mr->body);
            ofp_fsr->table_id = flow_stats_req_data->table_id;
            ofp_fsr->out_port = htonl(flow_stats_req_data->out_port);
            ofp_fsr->out_group = htonl(flow_stats_req_data->out_group);
            ofp_fsr->cookie = 0x0;
            ofp_fsr->cookie_mask = 0x0;
            ofp_fsr->match.type = htons(OFPMT_OXM);
            ofp_fsr->match.length = htons(4);
            memset(ofp_fsr->pad, 0x0, 3);
            memset(ofp_fsr->pad2, 0x0, 4);

            total_len += sizeof(struct ofp13_flow_stats_request);
            break;
        }
        case OFPMP_PORT_STATS:
        {
            port_stats_req_data_t *port_stats_req_data = (port_stats_req_data_t *)(stats_req_info->data);
            struct ofp13_port_stats_request *ofp_psr = (struct ofp13_port_stats_request *)(ofp_mr->body);
            ofp_psr->port_no = htonl(port_stats_req_data->port_no);
            memset(ofp_psr->pad, 0x0, 4);

            total_len += sizeof(struct ofp13_port_stats_request);
            break;
        }
        default:
        {
             break;
        }
    }

    ofp_mr->header.length = htons(total_len);
    return send_of_msg(sw, total_len);
}

static INT4 of13_msg_multipart_reply(gn_switch_t *sw, UINT1 *of_msg)
{
    gn_port_t new_sw_ports;
    struct ofp_multipart_reply *ofp_mr = (struct ofp_multipart_reply *)of_msg;
    UINT2 body_len = ntohs(ofp_mr->header.length) - sizeof(*ofp_mr);
    int loops;

    if (ntohs(ofp_mr->header.length) < sizeof(*ofp_mr))
        return GN_ERR;

    switch (htons(ofp_mr->type))
    {
        case OFPMP_DESC:
        {
            struct ofp_desc_stats *ods = (struct ofp_desc_stats *)(ofp_mr->body);
            memcpy(&sw->sw_desc, ods, sizeof(struct ofp_desc_stats));
            break;
        }

        case OFPMP_PORT_DESC:
        {
            struct ofp13_port *port = (struct ofp13_port *)(ofp_mr->body);
            loops = MAX_PORTS;
            UINT4 n_ports = 0;

            while (body_len && (--loops > 0))     //ÿһ���˿�ѭ��һ��
            {
                memset(&new_sw_ports, 0x0, sizeof(new_sw_ports));

                //Ĭ���������1000Mbps
                (sw->msg_driver.convertter->port_convertter)((UINT1 *)port, &new_sw_ports);
                new_sw_ports.stats.max_speed = 1000000;  //1073741824 = 1024^3, 1048576 = 1024^2

                body_len -= sizeof(struct ofp13_port);
                port = port + 1;

                if (new_sw_ports.port_no == OFPP13_LOCAL)
                {
                    sw->lo_port = new_sw_ports;
                    continue;
                }

                sw->ports[n_ports] = new_sw_ports;
                n_ports++;
            }

            sw->n_ports = n_ports;  //lo����
            break;
        }

        case OFPMP_TABLE_FEATURES:
        {
//            printf("table features reply\n");
            break;
        }

        case OFPMP_TABLE:
        {
//            printf("table stats reply\n");
            break;
        }

        case OFPMP_GROUP_FEATURES:
        {
//            printf("group features reply\n");
            break;
        }

        case OFPMP_METER_FEATURES:
        {
//            printf("meter features reply\n");
            break;
        }

        case OFPMP_FLOW:
        {
            of13_proc_flow_stats(sw, ofp_mr->body, body_len/sizeof(struct ofp13_flow_stats));
            break;
        }

        case OFPMP_PORT_STATS:
        {
            of13_proc_port_stats(sw, ofp_mr->body, body_len/sizeof(struct ofp13_port_stats));
            break;
        }

        default:
        {
//            printf("%s: mpart not handled\n", FN);
            break;
        }
    }

    return GN_OK;
}

static INT4 of13_msg_barrier_request(gn_switch_t *sw, UINT1 *bri_req)
{
    UINT2 total_len = sizeof(struct ofp_header);
    init_sendbuff(sw, OFP13_VERSION, OFPT13_BARRIER_REQUEST, total_len, 0);

    return send_of_msg(sw, total_len);
}

static INT4 of13_msg_barrier_reply(gn_switch_t *sw, UINT1 *of_msg)
{
    stats_req_info_t stats_req_info;
    port_stats_req_data_t port_stats_req_data;

    if(OFPCR_ROLE_SLAVE == g_controller_role)
    {
        return GN_OK;
    }

    //sw->msg_driver.msg_handler[OFPT13_PORT_MOD](sw, (UINT1 *)&omr);

    stats_req_info.flags = 0;
    stats_req_info.type = OFPMP_PORT_STATS;
    stats_req_info.data = (UINT1 *)&port_stats_req_data;
    port_stats_req_data.port_no = OFPP13_ANY;
    sw->msg_driver.msg_handler[OFPT13_MULTIPART_REQUEST](sw, (UINT1 *)&stats_req_info);

    //only for test
    of13_remove_all_flow(sw, of_msg);

    of13_table_miss(sw, of_msg);
    return GN_OK;
}

static INT4 of13_msg_queue_get_config_request(gn_switch_t *sw, UINT1 *of_msg)
{
    //todo
    return GN_OK;
}

static INT4 of13_msg_queue_get_config_reply(gn_switch_t *sw, UINT1 *of_msg)
{
    //todo
    return GN_OK;
}

static INT4 of13_msg_role_request(gn_switch_t *sw, UINT1 *role_req)
{
    UINT2 total_len = sizeof(struct ofp13_role_request);
    UINT1 *data = init_sendbuff(sw, OFP13_VERSION, OFPT13_ROLE_REQUEST, total_len, 0);
    role_req_info_t *new_role = (role_req_info_t *)role_req;
    struct ofp13_role_request *ofp13_role = (struct ofp13_role_request *)data;

    ofp13_role->role = htonl(new_role->role);
    ofp13_role->generation_id = gn_htonll(new_role->generation_id);

    return send_of_msg(sw, total_len);
}

static INT4 of13_msg_role_reply(gn_switch_t *sw, UINT1 *of_msg)
{
    //todo
    return GN_OK;
}

static INT4 of13_msg_get_async_request(gn_switch_t *sw, UINT1 *of_msg)
{
    //todo
    return GN_OK;
}

static INT4 of13_msg_get_async_reply(gn_switch_t *sw, UINT1 *of_msg)
{
    //todo
    return GN_OK;
}

static INT4 of13_msg_set_async(gn_switch_t *sw, UINT1 *of_msg)
{
    //todo
    return GN_OK;
}

static INT4 of13_msg_meter_mod(gn_switch_t *sw, UINT1 *metermod_req)
{
    meter_mod_req_info_t *meter_mod_req_info = (meter_mod_req_info_t *)metermod_req;
    UINT2 total_len = sizeof(struct ofp_meter_mod);
    UINT1 *data = init_sendbuff(sw, OFP13_VERSION, OFPT13_METER_MOD, total_len, meter_mod_req_info->xid);

    struct ofp_meter_mod *ofp13_meter = (struct ofp_meter_mod *)data;
    ofp13_meter->command = htons(meter_mod_req_info->command);
    ofp13_meter->flags = htons(meter_mod_req_info->meter->flags);
    ofp13_meter->meter_id = htonl(meter_mod_req_info->meter->meter_id);

    if(meter_mod_req_info->meter->type == OFPMBT_DROP)
    {
        struct ofp_meter_band_drop *ofp13_meter_band = (struct ofp_meter_band_drop *)(ofp13_meter->bands);
        ofp13_meter_band->type = htons(meter_mod_req_info->meter->type);
        ofp13_meter_band->len = htons(sizeof(struct ofp_meter_band_drop));
        ofp13_meter_band->rate = htonl(meter_mod_req_info->meter->rate);
        ofp13_meter_band->burst_size = htonl(meter_mod_req_info->meter->burst_size);

        total_len += sizeof(struct ofp_meter_band_drop);
    }
    else if(meter_mod_req_info->meter->type == OFPMBT_DSCP_REMARK)
    {
        struct ofp_meter_band_dscp_remark *ofp13_meter_band = (struct ofp_meter_band_dscp_remark *)(ofp13_meter->bands);
        ofp13_meter_band->type = htons(OFPMBT_DSCP_REMARK);
        ofp13_meter_band->len = htons(sizeof(struct ofp_meter_band_dscp_remark));
        ofp13_meter_band->rate = htonl(meter_mod_req_info->meter->rate);
        ofp13_meter_band->burst_size = htonl(meter_mod_req_info->meter->burst_size);
        ofp13_meter_band->prec_level = meter_mod_req_info->meter->prec_level;

        total_len += sizeof(struct ofp_meter_band_dscp_remark);
    }
    else
    {
        return GN_ERR;
    }

    ofp13_meter->header.length = htons(total_len);
    send_of_msg(sw, total_len);
    return GN_OK;
}

msg_handler_t of13_message_handler[OFP13_MAX_MSG] =
{
    of13_msg_hello,
    of13_msg_error,
    of13_msg_echo_request,
    of13_msg_echo_reply,
    of13_msg_experimenter,
    of13_msg_features_request,
    of13_msg_features_reply,
    of13_msg_get_config_request,
    of13_msg_get_config_reply,
    of13_msg_set_config,
    of13_msg_packet_in,
    of13_msg_flow_removed,
    of13_msg_port_status,
    of13_msg_packet_out,
    of13_msg_flow_mod,
    of13_msg_group_mod,
    of13_msg_port_mod,
    of13_msg_table_mod,
    of13_msg_multipart_request,
    of13_msg_multipart_reply,
    of13_msg_barrier_request,
    of13_msg_barrier_reply,
    of13_msg_queue_get_config_request,
    of13_msg_queue_get_config_reply,
    of13_msg_role_request,
    of13_msg_role_reply,
    of13_msg_get_async_request,
    of13_msg_get_async_reply,
    of13_msg_set_async,
    of13_msg_meter_mod
};