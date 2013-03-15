/*-
 * Copyright (c) 2013 Mark Johnston <markj@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer,
 * without modification, immediately at the beginning of the file.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <camlib.h>
#include <cam/scsi/scsi_pass.h>

int
main(int argc, char **argv)
{
	char buf[128];
	union ccb ccb;
	int i, fd, bufsize;

	fd = open(XPT_DEVICE, O_RDWR);
	if (fd < 0)
		err(1, "opening %s", XPT_DEVICE);

	memset(&ccb, 0, sizeof(ccb));

	ccb.ccb_h.path_id = CAM_XPT_PATH_ID;
	ccb.ccb_h.target_id = CAM_TARGET_WILDCARD;
	ccb.ccb_h.target_lun = CAM_LUN_WILDCARD;
	ccb.ccb_h.func_code = XPT_DEV_MATCH;

	bufsize = 100 * sizeof(struct dev_match_result);

	ccb.cdm.match_buf_len = bufsize;
	ccb.cdm.matches = malloc(bufsize);
	if (ccb.cdm.matches == NULL)
		err(1, "malloc");
	ccb.cdm.num_matches = 0;
	ccb.cdm.num_patterns = 0;
	ccb.cdm.pattern_buf_len = 0;

	if (ioctl(fd, CAMIOCOMMAND, &ccb))
		err(1, "ioctl");
	else if ((ccb.ccb_h.status & CAM_STATUS_MASK) != CAM_REQ_CMP ||
		    (ccb.cdm.status != CAM_DEV_MATCH_LAST &&
		    ccb.cdm.status != CAM_DEV_MATCH_MORE))
		errx(1, "CCB returned with error status");

	for (i = 0; i < ccb.cdm.num_matches; i++) {
		switch (ccb.cdm.matches[i].type) {
		case DEV_MATCH_DEVICE:
			if (ccb.cdm.matches[i].result.device_result.flags &
			    DEV_RESULT_UNCONFIGURED) {
				printf("matched unconfigured device\n");
				continue;
			}
			printf("matched device <%d,%d,%d>\n",
			    ccb.cdm.matches[i].result.device_result.path_id,
			    ccb.cdm.matches[i].result.device_result.target_id,
			    ccb.cdm.matches[i].result.device_result.target_lun);
			break;
		case DEV_MATCH_PERIPH:
			printf("matched periph %s%u at <%d,%d,%d>\n",
			    ccb.cdm.matches[i].result.periph_result.periph_name,
			    ccb.cdm.matches[i].result.periph_result.unit_number,
			    ccb.cdm.matches[i].result.periph_result.path_id,
			    ccb.cdm.matches[i].result.periph_result.target_id,
			    ccb.cdm.matches[i].result.periph_result.target_lun);
			break;
		case DEV_MATCH_BUS:
			printf("matched bus %s%u at <%d>\n",
			    ccb.cdm.matches[i].result.bus_result.dev_name,
			    ccb.cdm.matches[i].result.bus_result.unit_number,
			    ccb.cdm.matches[i].result.bus_result.path_id);
			break;
		}
	}

	return (0);
}
