#!pmake -V -W

# ===========================================================================
#
#                      PARADIGM SIMULATION INCORPORATED
#                 Copyright 1993 by Paradigm Simulation, Inc.
#
#     No part of this source code may be reproduced or distrubuted in any
#   form or by any means, or stored in a database or retrieval system,
#   without the prior written consent of Paradigm Simulation, Inc.
#
# ============================================================================


# PSI product specific common definitions
include $(WORKAREA)/src/demodefs

# PSI product specific rules
include $(WORKAREA)/src/demorules

DEMOINSTFILES = \


FILES = \
	2fokkerstart.aifc \
	diesel_boat.aifc \
	explosion1.aifc \
	flak.aifc \
	fokkergunMix1.aifc \
	fokkerrun.aifc \
	fokkerstop.aifc \
	goodyear.aifc \
	longrun.aifc \
	rev.aifc \
	rich.aifc \
	scream.aifc \
	smallexplosion1.aifc \
	smallexplosion2.aifc \
	steamtrain.aifc \
	water_explosion.aifc \
	wind.aifc


DEMOINCFILES  = *.aifc

# for making distributions
INSTMODE       = 444
DEMOINSTFILES  = $(DEMOINCFILES)
DEMOINSTDIR    = $(DEMOTOP)/vega/cimpact/data/audio_data
DEMOSRCDIR     = .
DEMOPARENTDIR  = .

oncedist: demoinstall


include $(COMMONRULES)

