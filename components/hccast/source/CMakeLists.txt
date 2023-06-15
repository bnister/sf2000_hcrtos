cmake_minimum_required(VERSION 3.0.0)
project(hccast)
set(CMAKE_BUILD_TYPE "release")

message(STATUS "Install Prefix (CMAKE_INSTALL_PREFIX):     ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Binary Directory (BIN_INSTALL_DIR):        ${BIN_INSTALL_DIR}")
message(STATUS "Library Directory (LIB_INSTALL_DIR):       ${LIB_INSTALL_DIR}")
message(STATUS "Include Directory (INCLUDE_INSTALL_DIR):   ${INCLUDE_INSTALL_DIR}")
message(STATUS "Documentation Directory (DOC_INSTALL_DIR): ${DOC_INSTALL_DIR}")
message(STATUS "Man Pages Directory (MAN_INSTALL_DIR):     ${MAN_INSTALL_DIR}")
message(STATUS "Build Type (CMAKE_BUILD_TYPE):             ${CMAKE_BUILD_TYPE}")
message(STATUS "Build static libraries:                    ${BUILD_STATIC_LIBS}")
message(STATUS "Build shared library:                      ${BUILD_SHARED_LIBS}")
message(STATUS "Project source dir::                       ${PROJECT_SOURCE_DIR}")

if(HCRTOS)
add_definitions(-DHC_RTOS -O1)
set(LIB_TYPE STATIC)
else()
add_definitions(-DHC_LINUX -O1)
set(LIB_TYPE SHARED)
endif()

include_directories(
                    ./inc
                    ./udhcp
                    ./common
                    ./httpd
                    ./wifi_mgr
                    ./aircast
                    ./dlna
                    ./miracast
                    ./udhcp
                    ./um
                   )

file(GLOB LIBHCCAST_COMMON
     common/hccast_dsc.c
     common/hccast_av.c
     common/hccast_log.c
     )

if(HCRTOS)
	file (GLOB LIBHCCAST_NETWORK
	     common/hccast_net.c
	     wifi_mgr/hccast_wifi_mgr.c
	     wifi_mgr/hccast_wifi_ctrl.c
	     wifi_mgr/hccast_p2p_ctrl_wext.c
	    )
else()
	file (GLOB LIBHCCAST_NETWORK
	     common/hccast_net.c
	     wifi_mgr/hccast_wifi_mgr.c
	     wifi_mgr/hccast_wifi_ctrl.c
	     wifi_mgr/hccast_p2p_ctrl.c
	    )

endif()

file (GLOB LIBHCCAST_DHCP
        udhcp/options.c
        udhcp/socket.c
        udhcp/packet.c
        udhcp/pidfile.c
        udhcp/dhcpd.c
        udhcp/arpping.c
        udhcp/files.c
        udhcp/leases.c
        udhcp/serverpacket.c
        udhcp/dhcpc.c
        udhcp/clientpacket.c
        udhcp/script.c
    )

file(GLOB LIBHCCAST_UM
     um/hccast_um_api.c
     um/hccast_um_avplayer.c
     um/hccast_aum_service.c
     um/hccast_ium_service.c
    )
    
if(AIRMIRROR_P30)
	add_definitions(-DAIRMIRROR_P30)
endif()

if(SOC_HC15XX)
	add_definitions(-DSOC_HC15XX)
endif()

if(WEB_CSTM_P1)
	add_definitions(-DWEB_CSTM_P1)
endif()

add_library(hccast-com ${LIB_TYPE} ${LIBHCCAST_COMMON})
install(TARGETS hccast-com LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

if(NETWORK_API)
	add_library(hccast-net ${LIB_TYPE} ${LIBHCCAST_NETWORK} ${LIBHCCAST_DHCP})
	install(TARGETS hccast-net LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
endif()

if(HCCAST_AIRCAST)
	file(GLOB LIBHCCAST_AIRCAST
     		aircast/hccast_air_service.c
     		aircast/hccast_air_avplayer.c
     		aircast/hccast_air_decoder.c
		aircast/hccast_air_api.c	
    	)
	add_definitions(-DSUPPORT_AIRCAST)
endif()

if(HCCAST_AIRCAST_MIRROR_ONLY)
	add_definitions(-DAIRCAST_SUPPORT_MIRROR_ONLY)
endif()

if(HCCAST_DLNA)
	file(GLOB LIBHCCAST_DLNA
     		dlna/hccast_dlna_service.c
     		dlna/hccast_dlna_player.c
    	)
	add_definitions(-DSUPPORT_DLNA)
endif()

if(HCCAST_MIRACAST)
	file(GLOB LIBHCCAST_MIRACAST
     		miracast/hccast_mira_service.c
     		miracast/hccast_mira_avplayer.c
    	)
	add_definitions(-DSUPPORT_MIRACAST)
endif()

if(HCCAST_UM)
	add_definitions(-DSUPPORT_UM)
	add_library(hccast-um ${LIB_TYPE} ${LIBHCCAST_UM})
	install(TARGETS hccast-um LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
endif()

if(WIRELESS_CASTING)
	file(GLOB LIBHCCAST_WIRELESS
		common/hccast_media.c
		common/hccast_scene.c
		httpd/hccast_httpd_service.c
		httpd/hccast_web_interface.c
		httpd/hccast_web_upgrade_download.c
	)
	add_library(hccast-wl ${LIB_TYPE} ${LIBHCCAST_WIRELESS} ${LIBHCCAST_DLNA} ${LIBHCCAST_AIRCAST} ${LIBHCCAST_MIRACAST})
	install(TARGETS hccast-wl LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
endif()

if(HCCAST_IPTV)
	file(GLOB LIBHCCAST_IPTV
		iptv/hccast_iptv.c
	)
	add_library(hccast-iptv ${LIB_TYPE} ${LIBHCCAST_IPTV})
	install(TARGETS hccast-iptv LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
endif()

if(HCLINUX)
	if(HCCAST_AIRCAST)
		if(HCCAST_DLNA)
			if(HCCAST_MIRACAST)
				add_executable(hccast_simple_wireless app/hccast_simple_wireless.c)
				TARGET_LINK_LIBRARIES(hccast_simple_wireless
							pthread
							cjson
							wpa_client
							hccast-com
							hccast-net
							hccast-wl
							mdns
							ffplayer
							dlna
							miracast
							crypto
							ssl)
				install(TARGETS hccast_simple_wireless RUNTIME DESTINATION bin)
			endif()
		endif()
	endif()

	if(HCCAST_UM)
		add_executable(hccast_simple_um app/hccast_simple_um.c)
		TARGET_LINK_LIBRARIES(hccast_simple_um
					pthread
					hccast-com
					hccast-um
					usb-1.0
					ssl
					crypto)
		install(TARGETS hccast_simple_um RUNTIME DESTINATION bin)
	endif()

endif()

if(HCCAST_ZLINK)
add_executable(hccast_simple_zlink app/hccast_simple_zlink.c)
TARGET_LINK_LIBRARIES(hccast_simple_zlink
			zlink
			pthread)
install(TARGETS hccast_simple_zlink RUNTIME DESTINATION bin)
endif()
