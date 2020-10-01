/*
 * Copyright (C) 2019 by Sukchan Lee <acetcom@gmail.com>
 *
 * This file is part of Open5GS.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "ogs-app.h"

static ogs_app_context_t self;

static int initialized = 0;

static void app_context_prepare(void);

int ogs_app_context_init(void)
{
    ogs_assert(initialized == 0);

    memset(&self, 0, sizeof(ogs_app_context_t));

    app_context_prepare();

    initialized = 1;

    return OGS_OK;
}

void ogs_app_context_final(void)
{
    ogs_assert(initialized == 1);

    if (self.document) {
        yaml_document_delete(self.document);
        free(self.document);
    }

    if (self.pollset)
        ogs_pollset_destroy(self.pollset);
    if (self.timer_mgr)
        ogs_timer_mgr_destroy(self.timer_mgr);
    if (self.queue)
        ogs_queue_destroy(self.queue);

    initialized = 0;
}

ogs_app_context_t *ogs_app()
{
    return &self;
}

static void recalculate_pool_size(void)
{
#define MAX_NUM_OF_TUNNEL       3   /* Num of Tunnel per Bearer */
    self.pool.sess = self.max.ue * OGS_MAX_NUM_OF_SESS;
    self.pool.bearer = self.pool.sess * OGS_MAX_NUM_OF_BEARER;
    self.pool.tunnel = self.pool.bearer * MAX_NUM_OF_TUNNEL;

#define MAX_NUM_OF_TIMER        16
    self.pool.timer = self.max.ue * MAX_NUM_OF_TIMER;

    self.pool.nf = self.max.gnb;
    self.pool.packet = self.max.ue * OGS_MAX_NUM_OF_PACKET_BUFFER;

#define MAX_NUM_OF_SOCKET       4   /* Num of socket per NF */
    self.pool.socket = self.pool.nf * MAX_NUM_OF_SOCKET;

#define MAX_NUM_OF_XACT         8
    self.pool.gtp_xact = self.max.ue * MAX_NUM_OF_XACT;
    self.pool.gtp_node = self.pool.nf;

    self.pool.pfcp_xact = self.max.ue * MAX_NUM_OF_XACT;
    self.pool.pfcp_node = self.pool.nf;

#define MAX_NUM_OF_NF_SERVICE   16  /* Num of NF Service per NF Instance */
#define MAX_NUM_OF_SBI_MESSAGE  8   /* Num of HTTP(s) Request/Response per NF */
#define MAX_NUM_OF_NF_SUBSCRIPTION  4 /* Num of Subscription per NF */
    self.pool.nf_service = self.pool.nf * MAX_NUM_OF_NF_SERVICE;
    self.pool.sbi_message = self.pool.nf * MAX_NUM_OF_SBI_MESSAGE;
    self.pool.nf_subscription = self.pool.nf * MAX_NUM_OF_NF_SUBSCRIPTION;

#define MAX_EVENT_POOL          32
    self.pool.event = MAX_EVENT_POOL;

#define MAX_CSMAP_POOL          128
    self.pool.csmap = MAX_CSMAP_POOL;   /* Num of TAI-LAI Mapping Table */
}

static void regenerate_all_timer_duration(void)
{
    ogs_assert(self.time.message.duration);

    self.time.message.sbi.client_wait_duration = self.time.message.duration;
    self.time.message.sbi.connection_deadline =
        self.time.message.sbi.client_wait_duration + ogs_time_from_sec(1);
    self.time.message.sbi.nf_register_interval =
        ogs_max(ogs_time_from_sec(3),
            self.time.message.sbi.client_wait_duration + ogs_time_from_sec(1));
    self.time.message.sbi.nf_register_interval_in_exception =
                ogs_time_from_msec(300);

#define PFCP_N1_RESPONSE_RETRY_COUNT  3
    self.time.message.pfcp.n1_response_rcount = PFCP_N1_RESPONSE_RETRY_COUNT;
    self.time.message.pfcp.t1_response_duration =
        (self.time.message.duration /
         (self.time.message.pfcp.n1_response_rcount + 1));
    ogs_assert(self.time.message.pfcp.t1_response_duration);

#define PFCP_N1_HOLDING_RETRY_COUNT 1
    self.time.message.pfcp.n1_holding_rcount = PFCP_N1_HOLDING_RETRY_COUNT;
    self.time.message.pfcp.t1_holding_duration =
        self.time.message.pfcp.n1_response_rcount *
        self.time.message.pfcp.t1_response_duration;
    ogs_assert(self.time.message.pfcp.t1_holding_duration);

    self.time.message.pfcp.association_interval =
        ogs_max(ogs_time_from_sec(3),
            self.time.message.sbi.client_wait_duration + ogs_time_from_sec(1));

    self.time.message.pfcp.no_heartbeat_duration =
        ogs_max(ogs_time_from_sec(10),
            self.time.message.sbi.client_wait_duration + ogs_time_from_sec(1));

#define GTP_N3_RESPONSE_RETRY_COUNT  3
    self.time.message.gtp.n3_response_rcount = GTP_N3_RESPONSE_RETRY_COUNT;
    self.time.message.gtp.t3_response_duration =
        (self.time.message.duration /
         (self.time.message.gtp.n3_response_rcount + 1));
    ogs_assert(self.time.message.gtp.t3_response_duration);

#define GTP_N3_HOLDING_RETRY_COUNT 1
    self.time.message.gtp.n3_holding_rcount = GTP_N3_HOLDING_RETRY_COUNT;
    self.time.message.gtp.t3_holding_duration =
        self.time.message.gtp.n3_response_rcount *
        self.time.message.gtp.t3_response_duration;
    ogs_assert(self.time.message.gtp.t3_holding_duration);
}

static void app_context_prepare(void)
{
#define USRSCTP_LOCAL_UDP_PORT      9899
    self.usrsctp.udp_port = USRSCTP_LOCAL_UDP_PORT;

#define MAX_NUM_OF_UE               1024    /* Num of UE per AMF/MME */
#define MAX_NUM_OF_GNB              32      /* Num of gNB per AMF/MME */

    self.max.gnb = MAX_NUM_OF_GNB;
    self.max.ue = MAX_NUM_OF_UE;

    ogs_pkbuf_default_init(&self.pool.defconfig);

    recalculate_pool_size();

    /* 10 second */
    self.time.nf_instance.heartbeat_interval = 10;
    self.time.nf_instance.no_heartbeat_margin = 1;

    /* 3600 seconds = 1 hour */
    self.time.nf_instance.validity_duration = 3600;

    /* 86400 seconds = 1 day */
    self.time.subscription.validity_duration = 86400;

    /* Message Wait Duration : 2 seconds */
    self.time.message.duration = ogs_time_from_sec(2);

    regenerate_all_timer_duration();
}
 
static int app_context_validation(void)
{
    if (self.parameter.no_ipv4 == 1 &&
        self.parameter.no_ipv6 == 1) {
        ogs_error("Both `no_ipv4` and `no_ipv6` set to `true` in `%s`",
                self.file);
        return OGS_ERROR;
    }

    if (self.time.nf_instance.validity_duration == 0) {
        ogs_error("NF Instance validity-time should not 0");
        ogs_error("time:");
        ogs_error("  nf_instance:");
        ogs_error("    validity: 0");

        return OGS_ERROR;
    }

    return OGS_OK;
}

int ogs_app_context_parse_config(void)
{
    int rv;
    yaml_document_t *document = NULL;
    ogs_yaml_iter_t root_iter;

    document = self.document;
    ogs_assert(document);

    ogs_yaml_iter_init(&root_iter, document);
    while (ogs_yaml_iter_next(&root_iter)) {
        const char *root_key = ogs_yaml_iter_key(&root_iter);
        ogs_assert(root_key);
        if (!strcmp(root_key, "db_uri")) {
            self.db_uri = ogs_yaml_iter_value(&root_iter);
        } else if (!strcmp(root_key, "logger")) {
            ogs_yaml_iter_t logger_iter;
            ogs_yaml_iter_recurse(&root_iter, &logger_iter);
            while (ogs_yaml_iter_next(&logger_iter)) {
                const char *logger_key = ogs_yaml_iter_key(&logger_iter);
                ogs_assert(logger_key);
                if (!strcmp(logger_key, "file")) {
                    self.logger.file = ogs_yaml_iter_value(&logger_iter);
                } else if (!strcmp(logger_key, "level")) {
                    self.logger.level =
                        ogs_yaml_iter_value(&logger_iter);
                } else if (!strcmp(logger_key, "domain")) {
                    self.logger.domain =
                        ogs_yaml_iter_value(&logger_iter);
                }
            }
        } else if (!strcmp(root_key, "parameter")) {
            ogs_yaml_iter_t parameter_iter;
            ogs_yaml_iter_recurse(&root_iter, &parameter_iter);
            while (ogs_yaml_iter_next(&parameter_iter)) {
                const char *parameter_key = ogs_yaml_iter_key(&parameter_iter);
                ogs_assert(parameter_key);
                if (!strcmp(parameter_key, "no_hss")) {
                    self.parameter.no_hss =
                        ogs_yaml_iter_bool(&parameter_iter);
                } else if (!strcmp(parameter_key, "no_mme")) {
                    self.parameter.no_mme =
                        ogs_yaml_iter_bool(&parameter_iter);
                } else if (!strcmp(parameter_key, "no_sgwu")) {
                    self.parameter.no_sgwu =
                        ogs_yaml_iter_bool(&parameter_iter);
                } else if (!strcmp(parameter_key, "no_sgwc")) {
                    self.parameter.no_sgwc =
                        ogs_yaml_iter_bool(&parameter_iter);
                } else if (!strcmp(parameter_key, "no_sgw")) {
                    self.parameter.no_sgw =
                        ogs_yaml_iter_bool(&parameter_iter);
                } else if (!strcmp(parameter_key, "no_pgw")) {
                    self.parameter.no_pgw =
                        ogs_yaml_iter_bool(&parameter_iter);
                } else if (!strcmp(parameter_key, "no_pcrf")) {
                    self.parameter.no_pcrf =
                        ogs_yaml_iter_bool(&parameter_iter);
                } else if (!strcmp(parameter_key, "no_nrf")) {
                    self.parameter.no_nrf =
                        ogs_yaml_iter_bool(&parameter_iter);
                } else if (!strcmp(parameter_key, "no_amf")) {
                    self.parameter.no_amf =
                        ogs_yaml_iter_bool(&parameter_iter);
                } else if (!strcmp(parameter_key, "no_smf")) {
                    self.parameter.no_smf =
                        ogs_yaml_iter_bool(&parameter_iter);
                } else if (!strcmp(parameter_key, "no_upf")) {
                    self.parameter.no_upf =
                        ogs_yaml_iter_bool(&parameter_iter);
                } else if (!strcmp(parameter_key, "no_ausf")) {
                    self.parameter.no_ausf =
                        ogs_yaml_iter_bool(&parameter_iter);
                } else if (!strcmp(parameter_key, "no_udm")) {
                    self.parameter.no_udm =
                        ogs_yaml_iter_bool(&parameter_iter);
                } else if (!strcmp(parameter_key, "no_udr")) {
                    self.parameter.no_udr =
                        ogs_yaml_iter_bool(&parameter_iter);
                } else if (!strcmp(parameter_key, "no_ipv4")) {
                    self.parameter.no_ipv4 =
                        ogs_yaml_iter_bool(&parameter_iter);
                } else if (!strcmp(parameter_key, "no_ipv6")) {
                    self.parameter.no_ipv6 =
                        ogs_yaml_iter_bool(&parameter_iter);
                } else if (!strcmp(parameter_key, "prefer_ipv4")) {
                    self.parameter.prefer_ipv4 =
                        ogs_yaml_iter_bool(&parameter_iter);
                } else if (!strcmp(parameter_key, "multicast")) {
                    self.parameter.multicast =
                        ogs_yaml_iter_bool(&parameter_iter);
                } else if (!strcmp(parameter_key, "no_slaac")) {
                    self.parameter.no_slaac =
                        ogs_yaml_iter_bool(&parameter_iter);
                } else
                    ogs_warn("unknown key `%s`", parameter_key);
            }
        } else if (!strcmp(root_key, "sctp")) {
            ogs_yaml_iter_t sctp_iter;
            ogs_yaml_iter_recurse(&root_iter, &sctp_iter);
            while (ogs_yaml_iter_next(&sctp_iter)) {
                const char *sctp_key = ogs_yaml_iter_key(&sctp_iter);
                ogs_assert(sctp_key);
                if (!strcmp(sctp_key, "heartbit_interval")) {
                    const char *v = ogs_yaml_iter_value(&sctp_iter);
                    if (v) self.sockopt.sctp.heartbit_interval = atoi(v);
                } else if (!strcmp(sctp_key, "rto_initial")) {
                    const char *v = ogs_yaml_iter_value(&sctp_iter);
                    if (v) self.sockopt.sctp.rto_initial = atoi(v);
                } else if (!strcmp(sctp_key, "rto_min")) {
                    const char *v = ogs_yaml_iter_value(&sctp_iter);
                    if (v) self.sockopt.sctp.rto_min = atoi(v);
                } else if (!strcmp(sctp_key, "rto_max")) {
                    const char *v = ogs_yaml_iter_value(&sctp_iter);
                    if (v) self.sockopt.sctp.rto_max = atoi(v);
                } else if (!strcmp(sctp_key, "max_num_of_ostreams")) {
                    const char *v = ogs_yaml_iter_value(&sctp_iter);
                    if (v)
                        self.sockopt.sctp.max_num_of_ostreams = atoi(v);
                } else if (!strcmp(sctp_key, "max_num_of_istreams")) {
                    const char *v = ogs_yaml_iter_value(&sctp_iter);
                    if (v)
                        self.sockopt.sctp.max_num_of_istreams = atoi(v);
                } else if (!strcmp(sctp_key, "max_attempts")) {
                    const char *v = ogs_yaml_iter_value(&sctp_iter);
                    if (v) self.sockopt.sctp.max_attempts = atoi(v);
                } else if (!strcmp(sctp_key, "max_initial_timeout")) {
                    const char *v = ogs_yaml_iter_value(&sctp_iter);
                    if (v)
                        self.sockopt.sctp.max_initial_timeout = atoi(v);
                } else if (!strcmp(sctp_key, "usrsctp_udp_port")) {
                    const char *v = ogs_yaml_iter_value(&sctp_iter);
                    if (v) self.usrsctp.udp_port = atoi(v);
                } else
                    ogs_warn("unknown key `%s`", sctp_key);
            }
        } else if (!strcmp(root_key, "max")) {
            ogs_yaml_iter_t max_iter;
            ogs_yaml_iter_recurse(&root_iter, &max_iter);
            while (ogs_yaml_iter_next(&max_iter)) {
                const char *max_key = ogs_yaml_iter_key(&max_iter);
                ogs_assert(max_key);
                if (!strcmp(max_key, "ue")) {
                    const char *v = ogs_yaml_iter_value(&max_iter);
                    if (v) self.max.ue = atoi(v);
                } else if (!strcmp(max_key, "gnb") ||
                            !strcmp(max_key, "enb")) {
                    const char *v = ogs_yaml_iter_value(&max_iter);
                    if (v) self.max.gnb = atoi(v);
                } else
                    ogs_warn("unknown key `%s`", max_key);
            }

            recalculate_pool_size();
        } else if (!strcmp(root_key, "pool")) {
            ogs_yaml_iter_t pool_iter;
            ogs_yaml_iter_recurse(&root_iter, &pool_iter);
            while (ogs_yaml_iter_next(&pool_iter)) {
                const char *pool_key = ogs_yaml_iter_key(&pool_iter);
                ogs_assert(pool_key);
                if (!strcmp(pool_key, "128")) {
                    const char *v = ogs_yaml_iter_value(&pool_iter);
                    if (v)
                        self.pool.defconfig.cluster_128_pool = atoi(v);
                } else if (!strcmp(pool_key, "256")) {
                    const char *v = ogs_yaml_iter_value(&pool_iter);
                    if (v)
                        self.pool.defconfig.cluster_256_pool = atoi(v);
                } else if (!strcmp(pool_key, "512")) {
                    const char *v = ogs_yaml_iter_value(&pool_iter);
                    if (v)
                        self.pool.defconfig.cluster_512_pool = atoi(v);
                } else if (!strcmp(pool_key, "1024")) {
                    const char *v = ogs_yaml_iter_value(&pool_iter);
                    if (v)
                        self.pool.defconfig.cluster_1024_pool = atoi(v);
                } else if (!strcmp(pool_key, "2048")) {
                    const char *v = ogs_yaml_iter_value(&pool_iter);
                    if (v)
                        self.pool.defconfig.cluster_2048_pool = atoi(v);
                } else if (!strcmp(pool_key, "8192")) {
                    const char *v = ogs_yaml_iter_value(&pool_iter);
                    if (v)
                        self.pool.defconfig.cluster_8192_pool = atoi(v);
                } else if (!strcmp(pool_key, "big")) {
                    const char *v = ogs_yaml_iter_value(&pool_iter);
                    if (v)
                        self.pool.defconfig.cluster_big_pool = atoi(v);
                } else
                    ogs_warn("unknown key `%s`", pool_key);
            }
        } else if (!strcmp(root_key, "time")) {
            ogs_yaml_iter_t time_iter;
            ogs_yaml_iter_recurse(&root_iter, &time_iter);
            while (ogs_yaml_iter_next(&time_iter)) {
                const char *time_key = ogs_yaml_iter_key(&time_iter);
                ogs_assert(time_key);
                if (!strcmp(time_key, "nf_instance")) {
                    ogs_yaml_iter_t sbi_iter;
                    ogs_yaml_iter_recurse(&time_iter, &sbi_iter);

                    while (ogs_yaml_iter_next(&sbi_iter)) {
                        const char *sbi_key =
                            ogs_yaml_iter_key(&sbi_iter);
                        ogs_assert(sbi_key);

                        if (!strcmp(sbi_key, "heartbeat")) {
                            const char *v = ogs_yaml_iter_value(&sbi_iter);
                            if (v) self.time.nf_instance.heartbeat_interval =
                                        atoi(v);
                        } else if (!strcmp(sbi_key, "validity")) {
                            const char *v = ogs_yaml_iter_value(&sbi_iter);
                            if (v) self.time.nf_instance.validity_duration =
                                        atoi(v);
                        } else
                            ogs_warn("unknown key `%s`", sbi_key);
                    }
                } else if (!strcmp(time_key, "subscription")) {
                    ogs_yaml_iter_t sbi_iter;
                    ogs_yaml_iter_recurse(&time_iter, &sbi_iter);

                    while (ogs_yaml_iter_next(&sbi_iter)) {
                        const char *sbi_key =
                            ogs_yaml_iter_key(&sbi_iter);
                        ogs_assert(sbi_key);

                        if (!strcmp(sbi_key, "validity")) {
                            const char *v = ogs_yaml_iter_value(&sbi_iter);
                            if (v) self.time.subscription.validity_duration =
                                        atoi(v);
                        } else
                            ogs_warn("unknown key `%s`", sbi_key);
                    }
                } else if (!strcmp(time_key, "message")) {
                    ogs_yaml_iter_t msg_iter;
                    ogs_yaml_iter_recurse(&time_iter, &msg_iter);

                    while (ogs_yaml_iter_next(&msg_iter)) {
                        const char *msg_key =
                            ogs_yaml_iter_key(&msg_iter);
                        ogs_assert(msg_key);

                        if (!strcmp(msg_key, "duration")) {
                            const char *v = ogs_yaml_iter_value(&msg_iter);
                            if (v) {
                                self.time.message.duration = 
                                    ogs_time_from_msec(atoll(v));
                                regenerate_all_timer_duration();
                            }
                        } else
                            ogs_warn("unknown key `%s`", msg_key);
                    }
                } else
                    ogs_warn("unknown key `%s`", time_key);
            }
        }
    }

    rv = app_context_validation();
    if (rv != OGS_OK) return rv;

    return OGS_OK;
}