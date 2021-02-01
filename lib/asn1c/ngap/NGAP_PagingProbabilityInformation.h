/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "NGAP-IEs"
 * 	found in "../support/ngap-r16.4.0/38413-g40.asn"
 * 	`asn1c -pdu=all -fcompound-names -findirect-choice -fno-include-deps -no-gen-BER -no-gen-XER -no-gen-OER -no-gen-UPER`
 */

#ifndef	_NGAP_PagingProbabilityInformation_H_
#define	_NGAP_PagingProbabilityInformation_H_


#include <asn_application.h>

/* Including external dependencies */
#include <NativeEnumerated.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum NGAP_PagingProbabilityInformation {
	NGAP_PagingProbabilityInformation_p00	= 0,
	NGAP_PagingProbabilityInformation_p05	= 1,
	NGAP_PagingProbabilityInformation_p10	= 2,
	NGAP_PagingProbabilityInformation_p15	= 3,
	NGAP_PagingProbabilityInformation_p20	= 4,
	NGAP_PagingProbabilityInformation_p25	= 5,
	NGAP_PagingProbabilityInformation_p30	= 6,
	NGAP_PagingProbabilityInformation_p35	= 7,
	NGAP_PagingProbabilityInformation_p40	= 8,
	NGAP_PagingProbabilityInformation_p45	= 9,
	NGAP_PagingProbabilityInformation_p50	= 10,
	NGAP_PagingProbabilityInformation_p55	= 11,
	NGAP_PagingProbabilityInformation_p60	= 12,
	NGAP_PagingProbabilityInformation_p65	= 13,
	NGAP_PagingProbabilityInformation_p70	= 14,
	NGAP_PagingProbabilityInformation_p75	= 15,
	NGAP_PagingProbabilityInformation_p80	= 16,
	NGAP_PagingProbabilityInformation_p85	= 17,
	NGAP_PagingProbabilityInformation_p90	= 18,
	NGAP_PagingProbabilityInformation_p95	= 19,
	NGAP_PagingProbabilityInformation_p100	= 20
	/*
	 * Enumeration is extensible
	 */
} e_NGAP_PagingProbabilityInformation;

/* NGAP_PagingProbabilityInformation */
typedef long	 NGAP_PagingProbabilityInformation_t;

/* Implementation */
extern asn_per_constraints_t asn_PER_type_NGAP_PagingProbabilityInformation_constr_1;
extern asn_TYPE_descriptor_t asn_DEF_NGAP_PagingProbabilityInformation;
extern const asn_INTEGER_specifics_t asn_SPC_PagingProbabilityInformation_specs_1;
asn_struct_free_f PagingProbabilityInformation_free;
asn_struct_print_f PagingProbabilityInformation_print;
asn_constr_check_f PagingProbabilityInformation_constraint;
per_type_decoder_f PagingProbabilityInformation_decode_aper;
per_type_encoder_f PagingProbabilityInformation_encode_aper;

#ifdef __cplusplus
}
#endif

#endif	/* _NGAP_PagingProbabilityInformation_H_ */
#include <asn_internal.h>