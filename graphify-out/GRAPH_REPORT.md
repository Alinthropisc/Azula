# Graph Report - azula  (2026-11-08)

## Corpus Check
- 450 files · ~381,595 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 99 file(s) not represented in the graph (top: .in 24, .cmake 21, (none) 9)

## Summary
- 4105 nodes · 9599 edges · 252 communities (180 shown, 72 thin omitted)
- Extraction: 73% EXTRACTED · 27% INFERRED · 0% AMBIGUOUS · INFERRED: 2613 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Community Hubs (Navigation)
- ec_socket.c
- __ns_type
- get_decoder
- string
- ec.h
- __ns_rcode
- nameser.h
- ec_strings.c
- __ns_flag
- __ns_msg
- wdg
- __ns_class
- __ns_sect
- Azula — Project Rules for AI Agents
- __ns_opcode
- __ns_cert_types
- ns_tcp_tsig_state
- __ns_key_types
- ns_tsig_key
- __ns_update_operation
- Core header migration
- ec_gtk4.c
- ec_plugins.c
- azula
- harness.py
- ec_text_profile.c
- azula
- azula
- session_get
- ec_curses.c
- ec_usleep
- packet.lua
- ec_packet
- dns_spoof.c
- ec_threads
- ec_gtk3.c
- repoison_arp.c
- hook_point
- wdg_set_focus
- ec_log
- ec_gtk4_view_connections.c
- ec_gtk4_targets.c
- ec_gtk3_view_connections.c
- ec_gtk4_plugins.c
- ec_set.c
- ec_encryption.c
- ec_gtk_view_connections.c
- hook_add
- ec_gtk4_view_profiles.c
- wdg_get_ncols
- ec_gtk4_hosts.c
- wdg.c
- base.lua
- ec_curses_view_connections.c
- ec_resolv.c
- sslstrip.c
- ec_text.c
- ec_sslwrap.c
- ec_send.c
- ec_filter.c
- ec_ssh.c
- ec_mingw.c
- ec_gtk4_redirect.c
- ec_gtk3_view_profiles.c
- ec_sniff.c
- ec_log.c
- list.lua
- main
- pptp_chapms1.c
- dissect_add
- ec_gtk3_plugins.c
- ec_lua.c
- ip_addr_ntoa
- gtkui_build_widgets
- ec_gtk.c
- ef_compiler.c
- wdg_file.c
- ec_http.c
- ec_profiles.c
- ec_daemon.c
- ec_gtk4_dialogs.c
- ec_utils.c
- ec_send
- ec_redirect
- ec_gtk3
- ec_gtk3_redirect.c
- ec_conntrack.c
- gtkui_page_new
- mac_addr_ntoa
- ec_gtk4_view.c
- ec_gtk_plugins.c
- ec_gtk3_view.c
- wdg_dynlist.c
- ec_gtk3_hosts.c
- ec_gtk4_mitm.c
- ec_solaris.c
- get_decoder
- wdg_dialog.c
- ec_gtk_redirect.c
- describe getopt.getOpt
- mdns_spoof.c
- ec_gtk4
- ec_network.c
- wdg_compound.c
- wdg_menu.c
- refresh_connections
- string_ext.lua
- table_ext.lua
- session_put
- GSimpleAction
- io_ext.lua
- ec_interfaces.c
- describe table.new 
- ec_proto.h
- wdg_panel.c
- ec_main.c
- ec_gtk3_mitm.c
- gtkui_exit
- ec_mdns.c
- ec_gtk4_help.c
- ec_parser.c
- describe string.pad 
- set.lua
- getopt.c
- nbns_spoof.c
- main
- ec_dissect.c
- ec_gg.c
- find_ip.c
- wdg_window.c
- gtkui_page_new
- ef_test.c
- pptp_clear.c
- ec_thread_getpid
- ip_addr_cmp
- ec_profile_item_get_property
- gpointer
- ec_gtk_view_profiles.c
- getopt.lua
- ec_fddi.c
- ec_tr.c
- ec_asn1.c
- dissector_telnet
- describe table.clone_rename
- ec_gtk_help.c
- describe string.wrap 
- isolate.c
- ec_ip.c
- ec_file
- ip_addr_init
- ec_nbns.c
- ec_gtk3_help.c
- ec_sniff_bridge.c
- ec_conn_item_get_property
- on_right_click
- ec_gtk_mitm.c
- context when requiring the module
- specify string_ext
- describe string.finds 
- describe string.tfind 
- specify table_ext
- describe table.merge 
- on_plug_bind
- ec_gnu.c
- inject_ip6
- find_ettercap.c
- ec_signals.c
- ec_radius.c
- ec_plugin_item_class_init
- gtkui_page_new
- ec_target_item_get_property
- describe table.clone 
- describe table.invert 
- ec_cygwin.c
- arp_cop.c
- GTK4 interface test harness
- ec_strlc
- describe string.caps 
- describe string.chomp 
- describe string.escape_pattern 
- describe string.escape_shell 
- describe string.ltrim 
- describe string.ordinal_suffix 
- describe string.rtrim 
- describe string.trim 
- describe table.keys 
- describe table.values 
- tree.lua
- provision.sh
- gtkui_infobar_show
- context when requiring the module
- describe .. operator
- describe string.numbertosi 
- describe string.split 
- describe table.empty 
- describe table.size 
- describe table.sort 
- dummy.c
- mbox.lua
- dumper.lua
- lcs.lua
- memrchr
- cmakelint.sh
- geolite-update.sh
- get-mac.sh

## God Nodes (most connected - your core abstractions)
1. `ip_addr_ntoa()` - 149 edges
2. `ip_addr_cmp()` - 81 edges
3. `memcmp()` - 63 edges
4. `__ns_type` - 53 edges
5. `parse_options()` - 50 edges
6. `dissect_add()` - 49 edges
7. `wdg_set_focus()` - 42 edges
8. `wdg_create_object()` - 41 edges
9. `ec_usleep()` - 40 edges
10. `hook_add()` - 39 edges

## Surprising Connections (you probably didn't know these)
- `nbns_set_challenge()` --calls--> `memcmp()`  [INFERRED]
  plug-ins/nbns_spoof/nbns_spoof.c → engine/missing/memcmp.c
- `nbns_spoof_fini()` --calls--> `hook_del()`  [INFERRED]
  plug-ins/nbns_spoof/nbns_spoof.c → engine/ec_hook.c
- `nbns_spoof_init()` --calls--> `hook_add()`  [INFERRED]
  plug-ins/nbns_spoof/nbns_spoof.c → engine/ec_hook.c
- `pptp_clear_fini()` --calls--> `hook_del()`  [INFERRED]
  plug-ins/pptp_clear/pptp_clear.c → engine/ec_hook.c
- `pptp_clear_init()` --calls--> `hook_add()`  [INFERRED]
  plug-ins/pptp_clear/pptp_clear.c → engine/ec_hook.c

## Import Cycles
- None detected.

## Communities (252 total, 72 thin omitted)

### Community 1 - "__ns_type"
Cohesion: 0.04
Nodes (53): __ns_type, ns_t_a, ns_t_a6, ns_t_aaaa, ns_t_afsdb, ns_t_any, ns_t_apl, ns_t_atma (+45 more)

### Community 2 - "get_decoder"
Cohesion: 0.04
Nodes (36): add_aligner(), add_decoder(), data_init(), add_injector(), add_builder(), arp_init(), align_cook(), cook_init() (+28 more)

### Community 4 - "ec.h"
Cohesion: 0.10
Nodes (7): decode_pwd(), dissector_icq(), icq_init(), snmp_init(), dissector_TN3270(), ebcdic2ascii(), TN3270_init()

### Community 5 - "__ns_rcode"
Cohesion: 0.12
Nodes (17): __ns_rcode, ns_r_badkey, ns_r_badsig, ns_r_badtime, ns_r_badvers, ns_r_formerr, ns_r_max, ns_r_noerror (+9 more)

### Community 6 - "nameser.h"
Cohesion: 0.11
Nodes (13): ns_cert_types, ns_class, ns_flag, _ns_flagdata, mask, shift, ns_key_types, ns_msg (+5 more)

### Community 7 - "ec_strings.c"
Cohesion: 0.14
Nodes (13): option, flag, has_arg, val, decode_args(), encode_const(), encode_function(), encode_offset() (+5 more)

### Community 8 - "__ns_flag"
Cohesion: 0.17
Nodes (12): __ns_flag, ns_f_aa, ns_f_ad, ns_f_cd, ns_f_max, ns_f_opcode, ns_f_qr, ns_f_ra (+4 more)

### Community 9 - "__ns_msg"
Cohesion: 0.10
Nodes (18): __ns_msg, _counts, _eom, _flags, _id, _msg, _msg_ptr, _rrnum (+10 more)

### Community 10 - "wdg"
Cohesion: 0.06
Nodes (16): filter_walk_list(), curses_update(), build_filter_list(), curses_select_filter(), refresh_filter_list(), curses_delete_host(), curses_host_target1(), curses_host_target2() (+8 more)

### Community 11 - "__ns_class"
Cohesion: 0.22
Nodes (9): __ns_class, ns_c_2, ns_c_any, ns_c_chaos, ns_c_hs, ns_c_in, ns_c_invalid, ns_c_max (+1 more)

### Community 12 - "__ns_sect"
Cohesion: 0.22
Nodes (9): __ns_sect, ns_s_an, ns_s_ar, ns_s_max, ns_s_ns, ns_s_pr, ns_s_qd, ns_s_ud (+1 more)

### Community 13 - "Azula — Project Rules for AI Agents"
Cohesion: 0.25
Nodes (7): Azula — Project Rules for AI Agents, Design patterns & principles to apply (name them in comments where used), graphify, Hard rules (non-negotiable), Known legacy defects to fix on sight, Project, Verification (agent MUST run before finishing)

### Community 14 - "__ns_opcode"
Cohesion: 0.29
Nodes (7): __ns_opcode, ns_o_iquery, ns_o_max, ns_o_notify, ns_o_query, ns_o_status, ns_o_update

### Community 15 - "__ns_cert_types"
Cohesion: 0.33
Nodes (6): __ns_cert_types, cert_t_oid, cert_t_pgp, cert_t_pkix, cert_t_spki, cert_t_url

### Community 16 - "ns_tcp_tsig_state"
Cohesion: 0.33
Nodes (6): ns_tcp_tsig_state, counter, ctx, key, sig, siglen

### Community 17 - "__ns_key_types"
Cohesion: 0.40
Nodes (5): __ns_key_types, ns_kt_dh, ns_kt_dsa, ns_kt_private, ns_kt_rsa

### Community 18 - "ns_tsig_key"
Cohesion: 0.40
Nodes (5): ns_tsig_key, alg, data, len, name

### Community 19 - "__ns_update_operation"
Cohesion: 0.50
Nodes (4): __ns_update_operation, ns_uop_add, ns_uop_delete, ns_uop_max

### Community 21 - "ec_gtk4.c"
Cohesion: 0.09
Nodes (37): gtkui_input(), ec_iface_item_class_init(), ec_iface_item_init(), gtkui_about(), gtkui_cleanup(), gtkui_cleanup_shim(), gtkui_error(), gtkui_error_shim() (+29 more)

### Community 22 - "ec_plugins.c"
Cohesion: 0.16
Nodes (17): plugin_filter(), plugin_fini(), plugin_init(), plugin_is_activated(), plugin_list(), plugin_list_walk(), plugin_load_all(), plugin_load_single() (+9 more)

### Community 26 - "harness.py"
Cohesion: 0.05
Nodes (19): c2s(), checksum(), ether(), ipv4(), main(), s2c(), tcp(), Check (+11 more)

### Community 27 - "ec_text_profile.c"
Cohesion: 0.19
Nodes (8): only_mitm(), ec_poll_buffer(), ec_poll_in(), ec_poll_out(), detail_help(), detail_hosts(), detail_select(), text_profiles()

### Community 31 - "session_get"
Cohesion: 0.14
Nodes (20): Check_CONNECT(), dissector_imap(), dissector_irc(), dissector_iscsi(), _memmem(), dissector_mongodb(), dissector_mountd(), dissector_msn() (+12 more)

### Community 32 - "ec_curses.c"
Cohesion: 0.05
Nodes (47): capture_default_if(), del_ip_list(), ui_error(), bridged_sniff(), curses_cleanup(), curses_exit(), curses_fatal_error(), curses_file_write() (+39 more)

### Community 33 - "ec_usleep"
Cohesion: 0.11
Nodes (29): capture_stop(), hook_add(), hook_del(), plugin_kill_thread(), hook_init(), random_list(), scan_ip6_onlink(), scan_netmask() (+21 more)

### Community 34 - "packet.lua"
Cohesion: 0.05
Nodes (21): from_base64(), from_binary(), to_base64(), to_binary(), parse_auth(), handle_response(), inject_body(), pad_http_body() (+13 more)

### Community 35 - "ec_packet"
Cohesion: 0.04
Nodes (41): plugin_register(), parse_icmp(), plugin_load(), find_conn_fini(), find_conn_init(), parse_arp(), plugin_load(), fraggle_attack_init() (+33 more)

### Community 36 - "dns_spoof.c"
Cohesion: 0.20
Nodes (17): match_pattern(), dns_spoof(), dns_spoof_dump(), dns_spoof_fini(), dns_spoof_init(), get_spoofed_a(), get_spoofed_aaaa(), get_spoofed_mx() (+9 more)

### Community 37 - "ec_threads"
Cohesion: 0.08
Nodes (24): mitm_add(), arp_poisoning_confirm(), arp_poisoning_init(), arp_poisoning_start(), create_list(), create_silent_list(), dhcp_setup_options(), dhcp_spoofing_init() (+16 more)

### Community 38 - "ec_gtk3.c"
Cohesion: 0.10
Nodes (28): gtkui_about(), gtkui_autostart_switch(), gtkui_bridged_switch(), gtkui_cleanup(), gtkui_cleanup_shim(), gtkui_combo_enter(), gtkui_context_menu(), gtkui_dialog_enter() (+20 more)

### Community 39 - "repoison_arp.c"
Cohesion: 0.15
Nodes (11): is_mitm_active(), add_to_victims(), autoadd_fini(), autoadd_init(), parse_arp(), plugin_load(), plugin_load(), repoison_arp_fini() (+3 more)

### Community 40 - "hook_point"
Cohesion: 0.09
Nodes (21): dissector_kerberos(), checksum_shouldbe(), L3_checksum(), L4_checksum(), sum(), v4_checksum(), v6_checksum(), hook_point() (+13 more)

### Community 41 - "wdg_set_focus"
Cohesion: 0.16
Nodes (49): curses_bridged_sniff(), curses_error(), curses_file_open(), curses_interface(), curses_progress(), curses_setup(), curses_load_filter(), curses_manage_filters() (+41 more)

### Community 42 - "ec_log"
Cohesion: 0.06
Nodes (52): ec_ctime(), basename(), analyze(), analyze_info(), analyze_packet(), create_hosts_list(), conn_decode(), conn_table_create() (+44 more)

### Community 43 - "ec_gtk4_view_connections.c"
Cohesion: 0.08
Nodes (38): gtkui_page_attach_shortcut(), gtkui_plugins_detach(), add_conn_column(), conn_filter_match(), detail_field(), detail_heading(), detail_hostname(), ec_conn_item_class_init() (+30 more)

### Community 44 - "ec_gtk4_targets.c"
Cohesion: 0.10
Nodes (33): gtkui_message(), add_target1(), add_target2(), delete_from_target(), ec_target_item_class_init(), ec_target_item_init(), ec_target_item_new(), entry_text() (+25 more)

### Community 45 - "ec_gtk3_view_connections.c"
Cohesion: 0.11
Nodes (20): gtkui_page_attach_shortcut(), connfilter(), gtkui_connection_data(), gtkui_connection_data_attach(), gtkui_connection_data_detach(), gtkui_connection_data_split(), gtkui_connection_detail_destroy(), gtkui_connection_inject() (+12 more)

### Community 46 - "ec_gtk4_plugins.c"
Cohesion: 0.14
Nodes (21): add_plug_column(), ec_plugin_item_finalize(), ec_plugin_item_init(), ec_plugin_item_new(), gtkui_add_plugin(), gtkui_create_plug_array(), gtkui_plug_destroy(), gtkui_plugin_load() (+13 more)

### Community 47 - "ec_set.c"
Cohesion: 0.09
Nodes (38): clean_exit(), parse_options(), disable_sslmitm(), opt_set_format(), opt_set_regex(), set_address(), set_broadcast(), set_compress() (+30 more)

### Community 48 - "ec_encryption.c"
Cohesion: 0.05
Nodes (47): dissector_bgp(), CRC_checksum(), ccmp_decrypt(), get_AAD(), get_BZERO(), get_PN(), wpa_ccmp_decrypt(), make_key_128() (+39 more)

### Community 49 - "ec_gtk_view_connections.c"
Cohesion: 0.09
Nodes (31): conntrack_hook_conn_del(), user_inject(), inject_file(), gtkui_destroy_conndata(), gtkui_connection_data_attach(), gtkui_destroy_conndata(), connfilter(), gtkui_connection_data() (+23 more)

### Community 50 - "hook_add"
Cohesion: 0.07
Nodes (24): ip_addr_pton(), ec_strsplit_ipport(), ui_input(), set_netmask_cb(), dos_attack_init(), parse_arp(), parse_icmp6(), parse_tcp() (+16 more)

### Community 51 - "ec_gtk4_view_profiles.c"
Cohesion: 0.12
Nodes (17): add_prof_column(), detail_field(), detail_heading(), ec_profile_item_class_init(), ec_profile_item_init(), find_profile_item(), gtkui_kill_profiles(), gtkui_profile_selected() (+9 more)

### Community 52 - "wdg_get_ncols"
Cohesion: 0.10
Nodes (41): wdg_file_menu_create(), wdg_input_form_create(), wdg_create_list(), wdg_list_borders(), wdg_list_callback(), wdg_list_destroy(), wdg_list_driver(), wdg_list_get_focus() (+33 more)

### Community 53 - "ec_gtk4_hosts.c"
Cohesion: 0.06
Nodes (46): build_hosts_list(), del_hosts_list(), scan_load_hosts(), curses_scan(), load_hosts(), add_text_column(), ec_host_item_class_init(), ec_host_item_finalize() (+38 more)

### Community 54 - "wdg.c"
Cohesion: 0.14
Nodes (21): curses_init(), wdg_create_input(), wdg_input_borders(), wdg_input_consolidate(), wdg_input_destroy(), wdg_input_driver(), wdg_input_form_destroy(), wdg_input_get_focus() (+13 more)

### Community 55 - "base.lua"
Cohesion: 0.07
Nodes (13): _G.ileaves(), _G.inodes(), _G.leaves(), _G.memoize(), _G.nodes(), _G.pickle(), _G.prettytostring(), _G.warn() (+5 more)

### Community 56 - "ec_curses_view_connections.c"
Cohesion: 0.20
Nodes (11): curses_connection_data(), curses_connection_data_help(), curses_connection_help(), curses_connection_kill(), curses_connection_kill_wrapper(), curses_connection_purge(), curses_kill_connections(), refresh_connections() (+3 more)

### Community 57 - "ec_resolv.c"
Cohesion: 0.16
Nodes (12): fnv_32(), fnv_64(), resolv_cache_insert(), resolv_cache_search(), resolv_dns(), resolv_queue_pop(), resolv_queue_push(), resolv_thread_fini() (+4 more)

### Community 58 - "sslstrip.c"
Cohesion: 0.10
Nodes (23): set_blocking(), strndup(), Decode_Url(), Find_Url(), http_accept_thread(), http_child_thread(), http_create_ident(), http_get_peer() (+15 more)

### Community 59 - "ec_text.c"
Cohesion: 0.18
Nodes (12): ui_msg_flush(), text_cleanup(), text_help(), text_hosts_list(), text_interface(), text_profile_list(), text_redirects(), text_run_filter() (+4 more)

### Community 60 - "ec_sslwrap.c"
Cohesion: 0.08
Nodes (35): fingerprint_submit(), packet_allocate_object(), packet_create_object(), packet_destroy_object(), packet_disp_data(), packet_dup(), ec_redirect_lookup(), close_socket() (+27 more)

### Community 61 - "ec_send.c"
Cohesion: 0.13
Nodes (24): capture_only_incoming(), ec_build_link_layer(), send_arp(), send_dhcp_reply(), send_dns_reply(), send_icmp_redir(), send_L2_icmp6_echo(), send_L2_icmp6_echo_opt() (+16 more)

### Community 62 - "ec_filter.c"
Cohesion: 0.12
Nodes (29): cmp_eq(), cmp_geq(), cmp_gt(), cmp_leq(), cmp_lt(), cmp_neq(), compile_regex(), execute_assign() (+21 more)

### Community 63 - "ec_ssh.c"
Cohesion: 0.11
Nodes (16): blowfish_decrypt(), blowfish_init(), des3_decrypt(), des3_init(), get_bn(), put_bn(), read_packet(), rsa_private_decrypt() (+8 more)

### Community 64 - "ec_mingw.c"
Cohesion: 0.06
Nodes (21): get_local_path(), dn_comp(), dn_find(), ec_win_dlerror(), ec_win_dn_expand(), ec_win_get_ec_dir(), ec_win_get_user_dir(), ec_win_pcap_stop() (+13 more)

### Community 65 - "ec_gtk4_redirect.c"
Cohesion: 0.09
Nodes (20): ec_redir_item_class_init(), ec_redir_item_finalize(), ec_redir_item_init(), ec_redir_item_new(), ec_service_item_class_init(), ec_service_item_finalize(), ec_service_item_init(), ec_service_item_new() (+12 more)

### Community 66 - "ec_gtk3_view_profiles.c"
Cohesion: 0.08
Nodes (17): geoip_init(), geoip_open(), print_xml_escaped(), profile_convert_to_hostlist(), curses_profiles_convert(), curses_profiles_dump(), curses_profiles_help(), gtkui_kill_profiles() (+9 more)

### Community 67 - "ec_sniff.c"
Cohesion: 0.06
Nodes (52): capture_freeifs(), ec_globals_free(), user_kill(), free_plugin_list(), add_ip(), add_ip_list(), add_port(), compile_display_filter() (+44 more)

### Community 68 - "ec_log.c"
Cohesion: 0.06
Nodes (53): dissector_rcon(), debug_msg(), log_close(), log_info(), log_open(), log_packet(), log_stop(), log_write_header() (+45 more)

### Community 69 - "list.lua"
Cohesion: 0.12
Nodes (20): _G.filter(), _G.map(), concat(), depair(), elems(), enpair(), filter(), flatten() (+12 more)

### Community 70 - "main"
Cohesion: 0.18
Nodes (16): http_fields_init(), capture_getifs(), conf_dissectors(), init_structures(), load_conf(), sanity_checks(), search_entry(), search_section() (+8 more)

### Community 71 - "pptp_chapms1.c"
Cohesion: 0.09
Nodes (16): ip_addr_null(), parse_option(), parse_ppp(), plugin_load(), pptp_chapms1_fini(), pptp_chapms1_init(), parse_option(), parse_ppp() (+8 more)

### Community 72 - "dissect_add"
Cohesion: 0.07
Nodes (34): bgp_init(), cvs_init(), dns_init(), ftp_init(), http_init(), imap_init(), irc_init(), iscsi_init() (+26 more)

### Community 73 - "ec_gtk3_plugins.c"
Cohesion: 0.14
Nodes (13): gtkui_create_plug_array(), gtkui_load_plugin(), gtkui_plug_destroy(), gtkui_plugin_context(), gtkui_plugin_load(), gtkui_plugin_mgmt(), gtkui_plugins_attach(), gtkui_plugins_autostart() (+5 more)

### Community 74 - "ec_lua.c"
Cohesion: 0.08
Nodes (23): debug_init(), bug(), error_msg(), fatal_error(), libettercap_ui_cleanup(), set_lua_args(), set_lua_script(), signal_SEGV() (+15 more)

### Community 75 - "ip_addr_ntoa"
Cohesion: 0.09
Nodes (16): dissector_cvs(), dissector_ldap(), dissector_ospf(), dissector_postgresql(), hex_encode(), dissector_rip(), dissector_smb(), GetBinaryE() (+8 more)

### Community 76 - "gtkui_build_widgets"
Cohesion: 0.11
Nodes (12): build_iface_model(), gtkui_conf_get(), ec_iface_item_finalize(), ec_iface_item_get_property(), gtkui_build_widgets(), gtkui_install_accels(), gtkui_create_tab_menu(), on_autostart_toggled() (+4 more)

### Community 77 - "ec_gtk.c"
Cohesion: 0.07
Nodes (25): gtkui_conf_get(), gtkui_conf_read(), gtkui_conf_save(), gtkui_conf_set(), gtkui_about(), gtkui_cleanup(), gtkui_cleanup_shim(), gtkui_error() (+17 more)

### Community 78 - "ef_compiler.c"
Cohesion: 0.13
Nodes (11): compile_tree(), compiler_concat_conditions(), labels_to_offsets(), unfold_blk(), unfold_conds(), unfold_ifblk(), ef_debug(), add_data_segment() (+3 more)

### Community 80 - "wdg_file.c"
Cohesion: 0.23
Nodes (12): wdg_create_file(), wdg_file_borders(), wdg_file_callback(), wdg_file_destroy(), wdg_file_driver(), wdg_file_get_focus(), wdg_file_get_msg(), wdg_file_lost_focus() (+4 more)

### Community 81 - "ec_http.c"
Cohesion: 0.20
Nodes (19): Decode_Url(), dissector_http(), dumpRaw(), Find_Url(), Find_Url_Referer(), Get_Banner(), Parse_Basic_Auth(), Parse_Form() (+11 more)

### Community 82 - "ec_profiles.c"
Cohesion: 0.12
Nodes (20): is_open_port(), profile_add_host(), profile_add_user(), profile_parse(), profile_print(), profile_purge(), profile_purge_all(), profile_purge_local() (+12 more)

### Community 83 - "ec_daemon.c"
Cohesion: 0.15
Nodes (4): daemon_cleanup(), daemon_error(), daemon_init(), daemonize()

### Community 84 - "ec_gtk4_dialogs.c"
Cohesion: 0.25
Nodes (11): dialog_ctx_free(), dialog_parent(), gtkui_dialog_confirm(), gtkui_dialog_inform(), gtkui_filename_browse(), gtkui_input_full(), gtkui_toast_error(), on_alert_response() (+3 more)

### Community 85 - "ec_utils.c"
Cohesion: 0.17
Nodes (7): reset_logfile_owners(), drop_privs(), ec_plen_to_binary(), get_decode_len(), parse_iflist(), regain_privs(), regain_privs_atexit()

### Community 86 - "ec_send"
Cohesion: 0.09
Nodes (12): get_injector(), inject_buffer(), inject_protocol(), send_to_L3(), forward_unified_sniff(), decode_esp(), esp_init(), align_eth() (+4 more)

### Community 87 - "ec_redirect"
Cohesion: 0.16
Nodes (11): ec_redirect(), ec_redirect_cleanup(), ec_walk_redirect_services(), ec_walk_redirects(), register_redir_service(), set_redir_command(), ssl_wrap_fini(), curses_sslredir_create_lists() (+3 more)

### Community 88 - "ec_gtk3"
Cohesion: 0.13
Nodes (7): hostlistbutton_clicked(), mitmstopbutton_clicked(), scanbutton_clicked(), toggle_sniffing(), gtkui_show_shortcuts(), gtkui_start_sniffing(), gtkui_stop_sniffing()

### Community 89 - "ec_gtk3_redirect.c"
Cohesion: 0.23
Nodes (12): gtkui_sslredir_add(), gtkui_sslredir_af_changed(), gtkui_sslredir_attach(), gtkui_sslredir_close(), gtkui_sslredir_create_lists(), gtkui_sslredir_del(), gtkui_sslredir_del_all(), gtkui_sslredir_detach() (+4 more)

### Community 91 - "ec_conntrack.c"
Cohesion: 0.14
Nodes (19): connbuf_add(), connbuf_init(), connbuf_wipe(), conntrack_add(), conntrack_del(), conntrack_hash(), conntrack_hook(), conntrack_hook_packet_add() (+11 more)

### Community 92 - "gtkui_page_new"
Cohesion: 0.43
Nodes (7): connbuf_print(), conntrack_hook_conn_add(), gtkui_page_new(), gtkui_page_present(), clear_data_window(), gtkui_connection_data_join(), gtkui_connection_data_split()

### Community 93 - "mac_addr_ntoa"
Cohesion: 0.27
Nodes (16): fingerprint_search(), geoip_get_by_ip(), mac_addr_ntoa(), manuf_search(), print_host(), print_host_xml(), host_iptoa(), service_search() (+8 more)

### Community 94 - "ec_gtk4_view.c"
Cohesion: 0.06
Nodes (41): ascii_format(), bin_format(), ebcdic_format(), hex_format(), html_format(), set_format(), set_utf8_encoding(), text_format() (+33 more)

### Community 95 - "ec_gtk_plugins.c"
Cohesion: 0.16
Nodes (11): gtkui_create_plug_array(), gtkui_load_plugin(), gtkui_plug_destroy(), gtkui_plugin_load(), gtkui_plugin_mgmt(), gtkui_plugins_attach(), gtkui_plugins_autostart(), gtkui_plugins_detach() (+3 more)

### Community 96 - "ec_gtk3_view.c"
Cohesion: 0.24
Nodes (12): gtkui_input(), gtkui_add_target1(), gtkui_add_target2(), gtkui_show_stats(), gtkui_stats_attach(), gtkui_stop_stats(), gtkui_vis_method(), gtkui_vis_regex() (+4 more)

### Community 97 - "wdg_dynlist.c"
Cohesion: 0.17
Nodes (18): curses_profiles_local(), curses_profiles_remote(), refresh_profiles(), wdg_create_dynlist(), wdg_dynlist_add_callback(), wdg_dynlist_border(), wdg_dynlist_callback(), wdg_dynlist_get_focus() (+10 more)

### Community 98 - "ec_gtk3_hosts.c"
Cohesion: 0.25
Nodes (12): gtkui_button_callback(), gtkui_host_list(), gtkui_hosts_attach(), gtkui_hosts_destroy(), gtkui_hosts_detach(), gtkui_load_hosts(), gtkui_refresh_host_list(), gtkui_save_hosts() (+4 more)

### Community 99 - "ec_gtk4_mitm.c"
Cohesion: 0.28
Nodes (11): fields_grid(), gtkui_arp_poisoning(), gtkui_dhcp_spoofing(), gtkui_icmp_redir(), gtkui_mitm_stop(), gtkui_ndp_poisoning(), gtkui_port_stealing(), gtkui_start_mitm() (+3 more)

### Community 100 - "ec_solaris.c"
Cohesion: 0.05
Nodes (7): safe_free_mem(), get_iface_mtu(), get_iface_mtu(), check_tempaddr(), disable_interface_offload(), get_iface_mtu(), get_iface_mtu()

### Community 101 - "get_decoder"
Cohesion: 0.05
Nodes (30): capture(), decode_data(), del_decoder(), ec_decode(), find_entry(), get_decoder(), sort_decoders(), top_half() (+22 more)

### Community 102 - "wdg_dialog.c"
Cohesion: 0.21
Nodes (14): wdg_create_dialog(), wdg_dialog_add_callback(), wdg_dialog_border(), wdg_dialog_buttons(), wdg_dialog_callback(), wdg_dialog_get_focus(), wdg_dialog_get_msg(), wdg_dialog_get_size() (+6 more)

### Community 103 - "ec_gtk_redirect.c"
Cohesion: 0.26
Nodes (12): gtkui_sslredir_add(), gtkui_sslredir_af_changed(), gtkui_sslredir_attach(), gtkui_sslredir_close(), gtkui_sslredir_create_lists(), gtkui_sslredir_del(), gtkui_sslredir_del_all(), gtkui_sslredir_detach() (+4 more)

### Community 104 - "describe getopt.getOpt"
Cohesion: 0.12
Nodes (16): before, it allows the user to override the help option, it allows the user to override the version option, it provides a default help option, it provides a default version option, before, it captures a list of repeated option arguments, it diagnoses unrecognized options (+8 more)

### Community 105 - "mdns_spoof.c"
Cohesion: 0.21
Nodes (13): get_spoofed_a(), get_spoofed_aaaa(), get_spoofed_ptr(), get_spoofed_srv(), load_db(), mdns_spoof(), mdns_spoof_dump(), mdns_spoof_fini() (+5 more)

### Community 106 - "ec_gtk4"
Cohesion: 0.11
Nodes (12): gtkui_conf_read(), gtkui_conf_save(), gtkui_conf_set(), gtkui_init(), gtkui_setup(), gtkui_start(), hostlistbutton_clicked(), mitmstopbutton_clicked() (+4 more)

### Community 107 - "ec_network.c"
Cohesion: 0.16
Nodes (11): get_alignment(), ip_addr_get_prefix(), close_secondary_sources(), iface_by_mac(), l3_init(), network_init(), pcap_winit(), secondary_sources_init() (+3 more)

### Community 108 - "wdg_compound.c"
Cohesion: 0.23
Nodes (12): wdg_compound_border(), wdg_compound_destroy(), wdg_compound_dispatch(), wdg_compound_get_focus(), wdg_compound_get_focused(), wdg_compound_get_msg(), wdg_compound_lost_focus(), wdg_compound_move() (+4 more)

### Community 109 - "wdg_menu.c"
Cohesion: 0.18
Nodes (14): wdg_create_menu(), wdg_menu_close(), wdg_menu_driver(), wdg_menu_get_focus(), wdg_menu_get_msg(), wdg_menu_lost_focus(), wdg_menu_mouse_move(), wdg_menu_move() (+6 more)

### Community 110 - "refresh_connections"
Cohesion: 0.28
Nodes (15): conntrack_countrystr(), conntrack_flagstr(), conntrack_get(), conntrack_print(), conntrack_protostr(), conntrack_statusstr(), gtkui_connections_add(), refresh_connections() (+7 more)

### Community 111 - "string_ext.lua"
Cohesion: 0.16
Nodes (7): parse_args(), finds(), ltrim(), rtrim(), split(), tfind(), trim()

### Community 113 - "session_put"
Cohesion: 0.25
Nodes (9): dissector_ftp(), dissector_nntp(), dissector_pop(), dissector_rlogin(), session_del(), session_free(), session_get_and_del(), session_hash() (+1 more)

### Community 114 - "GSimpleAction"
Cohesion: 0.27
Nodes (11): gtkui_file_open(), gtkui_file_write(), gtkui_page_close_current(), gtkui_page_detach_current(), gtkui_page_left(), gtkui_page_right(), gtkui_pcap_filter(), gtkui_set_netmask() (+3 more)

### Community 115 - "io_ext.lua"
Cohesion: 0.16
Nodes (5): catdir(), catfile(), input_handle(), readlines(), slurp()

### Community 116 - "ec_interfaces.c"
Cohesion: 0.15
Nodes (11): select_curses_interface(), select_daemon_interface(), select_gtk_interface(), select_text_interface(), ui_register(), set_curses_interface(), set_daemon_interface(), set_gtk_interface() (+3 more)

### Community 117 - "describe table.new "
Cohesion: 0.14
Nodes (14): before, it returns a new table when nil is passed, it returns any table passed in, before, it returns a new table when nil is passed, it returns any table passed in, before, context when not setting a default (+6 more)

### Community 118 - "ec_proto.h"
Cohesion: 0.19
Nodes (6): ec_globals_alloc(), libettercap_init(), main(), START_TEST(), test_get_decoder_default(), ts_test_decode()

### Community 119 - "wdg_panel.c"
Cohesion: 0.23
Nodes (8): wdg_create_panel(), wdg_panel_border(), wdg_panel_get_focus(), wdg_panel_get_msg(), wdg_panel_lost_focus(), wdg_panel_print(), wdg_panel_redraw(), wdg_panel_resize()

### Community 120 - "ec_main.c"
Cohesion: 0.22
Nodes (4): libettercap_ui_init(), libettercap_ui_start(), ui_init(), ui_start()

### Community 121 - "ec_gtk3_mitm.c"
Cohesion: 0.55
Nodes (7): gtkui_arp_poisoning(), gtkui_dhcp_spoofing(), gtkui_icmp_redir(), gtkui_mitm_stop(), gtkui_ndp_poisoning(), gtkui_port_stealing(), gtkui_start_mitm()

### Community 122 - "gtkui_exit"
Cohesion: 0.28
Nodes (7): gtkui_conf_read(), gtkui_conf_save(), gtkui_conf_set(), gtkui_exit(), gtkui_init(), gtkui_setup(), gtkui_start()

### Community 123 - "ec_mdns.c"
Cohesion: 0.29
Nodes (7): dissector_dns(), dissector_mdns(), handle_ipv4_record(), handle_ipv6_record(), handle_srv_record(), mdns_init(), resolv_cache_insert_passive()

### Community 124 - "ec_gtk4_help.c"
Cohesion: 0.23
Nodes (5): gtkui_help(), gtkui_help_open(), on_help_bind(), on_help_selected(), on_help_setup()

### Community 126 - "describe string.pad "
Cohesion: 0.15
Nodes (13): before, it left pads a string to given width with spaces, it truncates a string to the given width, before, it left pads a string to the given negative width with spaces, it right pads a string to the given width with spaces, before, context when string is longer than given width (+5 more)

### Community 127 - "set.lua"
Cohesion: 0.40
Nodes (11): difference(), elems(), equal(), insert(), intersection(), member(), new(), propersubset() (+3 more)

### Community 128 - "getopt.c"
Cohesion: 0.26
Nodes (6): exchange(), getopt(), _getopt_internal(), getopt_long(), getopt_long_only(), main()

### Community 129 - "nbns_spoof.c"
Cohesion: 0.22
Nodes (10): get_spoofed_nbns(), load_db(), nbns_expand(), nbns_set_challenge(), nbns_spoof(), nbns_spoof_dump(), nbns_spoof_fini(), nbns_spoof_init() (+2 more)

### Community 130 - "main"
Cohesion: 0.23
Nodes (9): filter_init_mutex(), ef_exit(), ef_globals_alloc(), ef_globals_free(), main(), ef_usage(), parse_options(), load_constants() (+1 more)

### Community 131 - "ec_dissect.c"
Cohesion: 0.28
Nodes (5): dissect_del(), dissect_modify(), dissect_on_port(), dissect_on_port_level(), sslw_dissect_move()

### Community 132 - "ec_gg.c"
Cohesion: 0.43
Nodes (4): dissector_gg(), gg_get_status(), gg_get_version(), gg_init()

### Community 133 - "find_ip.c"
Cohesion: 0.39
Nodes (5): find_ip_init(), in_list(), plugin_load(), search_netmask(), search_targets()

### Community 134 - "wdg_window.c"
Cohesion: 0.23
Nodes (9): refresh_stats(), wdg_create_window(), wdg_window_border(), wdg_window_get_focus(), wdg_window_get_msg(), wdg_window_lost_focus(), wdg_window_print(), wdg_window_redraw() (+1 more)

### Community 135 - "gtkui_page_new"
Cohesion: 0.22
Nodes (5): gtkui_toast(), add_redir_column(), gtkui_sslredir_create_lists(), gtkui_sslredir_refresh_lists(), gtkui_sslredir_show()

### Community 136 - "ef_test.c"
Cohesion: 0.42
Nodes (7): escape_binary(), print_assign(), print_dec(), print_fop(), print_function(), print_inc(), print_test()

### Community 137 - "pptp_clear.c"
Cohesion: 0.27
Nodes (7): obfuscate_options(), parse_ecp(), parse_ipcp(), parse_lcp(), parse_option(), pptp_clear_fini(), pptp_clear_init()

### Community 138 - "ec_thread_getpid"
Cohesion: 0.11
Nodes (30): capture_start(), secondary_sources_foreach(), start_bridge_sniff(), start_unified_sniff(), stop_unified_sniff(), ec_thread_destroy(), ec_thread_getdesc(), ec_thread_getname() (+22 more)

### Community 139 - "ip_addr_cmp"
Cohesion: 0.12
Nodes (26): dissect_match(), hex_len(), ip_addr_cmp(), ip_addr_is_ours(), add_host(), get_response(), hosts_list_hook(), unified_check_forwarded() (+18 more)

### Community 140 - "ec_profile_item_get_property"
Cohesion: 0.43
Nodes (3): ec_profile_item_finalize(), ec_profile_item_get_property(), ec_profile_item_set_property()

### Community 141 - "gpointer"
Cohesion: 0.27
Nodes (5): on_af_changed(), on_redir_bind(), on_redir_setup(), on_service_bind(), on_service_setup()

### Community 142 - "ec_gtk_view_profiles.c"
Cohesion: 0.22
Nodes (7): gtkui_kill_profiles(), gtkui_profile_detail_destroy(), gtkui_profile_selected(), gtkui_profiles_attach(), gtkui_profiles_detach(), gtkui_show_profiles(), refresh_profiles()

### Community 143 - "getopt.lua"
Cohesion: 0.27
Nodes (7): getOpt(), makeOptions(), processArgs(), usage(), usageInfo(), version(), Parser:_init()

### Community 144 - "ec_fddi.c"
Cohesion: 0.29
Nodes (3): align_fddi(), build_fddi(), decode_fddi()

### Community 145 - "ec_tr.c"
Cohesion: 0.29
Nodes (3): align_tr(), build_tr(), decode_tr()

### Community 146 - "ec_asn1.c"
Cohesion: 0.36
Nodes (6): asn1_bit_string_to_long(), asn1_get_next(), asn1_get_oid(), asn1_parse_oid(), rotate_bits(), parse_krb5()

### Community 147 - "dissector_telnet"
Cohesion: 0.18
Nodes (9): convert_zeros(), dissector_telnet(), match_login_regex(), skip_telnet_command(), str_replace(), strcasestr(), good_page(), remote_browser() (+1 more)

### Community 148 - "describe table.clone_rename"
Cohesion: 0.20
Nodes (10): before, it does not perturb the value in the renamed key field, it renames during cloning, before, context when renaming some keys, it copies the subject, it diagnoses non-table arguments, it does not just return the subject (+2 more)

### Community 150 - "describe string.wrap "
Cohesion: 0.22
Nodes (9): before, it can indent the first line differently, it diagnoses indent greater than line width, it diagnoses non-string arguments, it honours a column width parameter, it inserts newlines to wrap a string, it supports indenting by a fixed number of columns, the original subject is not perturbed (+1 more)

### Community 152 - "isolate.c"
Cohesion: 0.40
Nodes (4): add_to_victims(), isolate_init(), parse_arp(), plugin_load()

### Community 154 - "ec_ip.c"
Cohesion: 0.17
Nodes (9): fingerprint_default(), fingerprint_push(), TTL_PREDICTOR(), decode_ip(), inject_ip(), ip_create_ident(), ip_create_session(), ip_match() (+1 more)

### Community 155 - "ec_file"
Cohesion: 0.12
Nodes (11): get_full_path(), filter_clear(), filter_unload(), curses_stop_filter(), gtkui_load_filter(), gtkui_stop_filter(), gtkui_load_filter(), gtkui_stop_filter() (+3 more)

### Community 156 - "ip_addr_init"
Cohesion: 0.13
Nodes (19): dhcp_add_profile(), dhcp_init(), dissector_dhcp(), get_dhcp_option(), put_dhcp_option(), inet_ntop4(), inet_ntop6(), ip_addr_cpy() (+11 more)

### Community 157 - "ec_nbns.c"
Cohesion: 0.50
Nodes (3): dissector_nbns(), nbns_expand(), nbns_init()

### Community 158 - "ec_gtk3_help.c"
Cohesion: 0.32
Nodes (3): gtkui_help(), gtkui_help_open(), gtkui_help_selected()

### Community 159 - "ec_sniff_bridge.c"
Cohesion: 0.50
Nodes (3): bridge_check_forwarded(), bridge_set_forwardable(), stop_bridge_sniff()

### Community 160 - "ec_conn_item_get_property"
Cohesion: 0.25
Nodes (3): ec_conn_item_finalize(), ec_conn_item_get_property(), uint_to_label()

### Community 162 - "ec_gtk_mitm.c"
Cohesion: 0.21
Nodes (12): mitm_set(), mitm_start(), mitm_stop(), set_mitm(), curses_start_mitm(), gtkui_arp_poisoning(), gtkui_dhcp_spoofing(), gtkui_icmp_redir() (+4 more)

### Community 163 - "context when requiring the module"
Cohesion: 0.25
Nodes (7): before, it does not override any other module access points, it returns the unextended package table, it splits package.config up, specify package_ext, before, context when requiring the module

### Community 164 - "specify string_ext"
Cohesion: 0.25
Nodes (7): before, it diagnoses non-string arguments, it returns a single argument without attempting formatting, the original subject is not perturbed, specify string_ext, before, describe string.format ()

### Community 165 - "describe string.finds "
Cohesion: 0.25
Nodes (8): before, it creates a list of pattern captures, it creates an empty list for a pattern without captures, it creates an empty list where no captures are matched, it diagnoses non-string arguments, it starts the search at a specified index into the subject, the original subject is not perturbed, describe string.finds ()

### Community 166 - "describe string.tfind "
Cohesion: 0.25
Nodes (8): before, it creates a list of pattern captures, it creates an empty list for a pattern without captures, it creates an empty list where no captures are matched, it diagnoses non-string arguments, it starts the search at a specified index into the subject, the original subject is not perturbed, describe string.tfind ()

### Community 167 - "specify table_ext"
Cohesion: 0.25
Nodes (7): before, it does not override any other module access points, it injects an enhanced sort function, it returns the unextended module table, specify table_ext, before, context when requiring the module

### Community 168 - "describe table.merge "
Cohesion: 0.25
Nodes (8): before, it copies t2 when t1 is empty, it diagnoses non-table arguments, it does not change t1 when t2 is empty, it does not create a whole new table, it gives precedence to values from t2, it merges keys from t2 into t1, describe table.merge ()

### Community 172 - "find_ettercap.c"
Cohesion: 0.25
Nodes (6): find_ettercap_fini(), find_ettercap_init(), parse_icmp(), parse_ip(), parse_tcp(), plugin_load()

### Community 177 - "gtkui_page_new"
Cohesion: 0.38
Nodes (5): gtkui_page_new(), gtkui_page_present(), gtkui_create_tab_menu(), gtkui_connection_data_join(), gtkui_show_profiles()

### Community 179 - "ec_target_item_get_property"
Cohesion: 0.43
Nodes (3): ec_target_item_finalize(), ec_target_item_get_property(), ec_target_item_set_property()

### Community 180 - "describe table.clone "
Cohesion: 0.29
Nodes (7): before, it diagnoses non-table arguments, it does copy the subject, it does not just return the subject, it only makes a shallow copy, the original subject is not perturbed, describe table.clone ()

### Community 181 - "describe table.invert "
Cohesion: 0.29
Nodes (7): before, it diagnoses non-table arguments, it inverts keys and values in the returned table, it is reversible, it returns a new table, it seems to copy a list of 1..n numbers, describe table.invert ()

### Community 183 - "arp_cop.c"
Cohesion: 0.33
Nodes (5): arp_cop_fini(), arp_cop_init(), arp_init_list(), parse_arp(), plugin_load()

### Community 184 - "GTK4 interface test harness"
Cohesion: 0.29
Nodes (6): Building locally without the harness, Checks, GTK4 interface test harness, Requirements, Running, What's here

### Community 185 - "ec_strlc"
Cohesion: 0.53
Nodes (6): ec_strlc(), ec_struc(), gtkui_sslredir_add_list(), gtkui_sslredir_add_service(), gtkui_sslredir_add_list(), gtkui_sslredir_add_service()

### Community 186 - "describe string.caps "
Cohesion: 0.33
Nodes (6): before, it capitalises words of a string, it changes only the first letter of each word, it diagnoses non-string arguments, the original subject is not perturbed, describe string.caps ()

### Community 187 - "describe string.chomp "
Cohesion: 0.33
Nodes (6): before, it diagnoses non-string arguments, it does not change a string with no trailing newline, it removes a single trailing newline from a string, the original subject is not perturbed, describe string.chomp ()

### Community 188 - "describe string.escape_pattern "
Cohesion: 0.33
Nodes (6): before, it diagnoses non-string arguments, it inserts a % before any non-alphanumeric in a string, legacy escapePattern call is the same function, the original subject is not perturbed, describe string.escape_pattern ()

### Community 189 - "describe string.escape_shell "
Cohesion: 0.33
Nodes (6): before, it diagnoses non-string arguments, it inserts a \ before any shell metacharacters, legacy escapeShell call is the same function, the original subject is not perturbed, describe string.escape_shell ()

### Community 190 - "describe string.ltrim "
Cohesion: 0.33
Nodes (6): before, it diagnoses non-string arguments, it removes whitespace from the start of a string, it supports custom removal patterns, the original subject is not perturbed, describe string.ltrim ()

### Community 191 - "describe string.ordinal_suffix "
Cohesion: 0.33
Nodes (6): before, it coerces string arguments to a number, it diagnoses non-numeric arguments, it returns the English suffix for a number, legacy ordinalSuffix call is the same function, describe string.ordinal_suffix ()

### Community 192 - "describe string.rtrim "
Cohesion: 0.33
Nodes (6): before, it diagnoses non-string arguments, it removes whitespace from the end of a string, it supports custom removal patterns, the original subject is not perturbed, describe string.rtrim ()

### Community 193 - "describe string.trim "
Cohesion: 0.33
Nodes (6): before, it diagnoses non-string arguments, it removes whitespace from each end of a string, it supports custom removal patterns, the original subject is not perturbed, describe string.trim ()

### Community 194 - "describe table.keys "
Cohesion: 0.33
Nodes (6): before, it diagnoses non-table arguments, it does not guarantee stable ordering, it makes a list of table keys, it returns an empty list when subject is empty, describe table.keys ()

### Community 195 - "describe table.values "
Cohesion: 0.33
Nodes (6): before, it diagnoses non-table arguments, it does guarantee stable ordering, it makes a list of table values, it returns an empty list when subject is empty, describe table.values ()

### Community 197 - "provision.sh"
Cohesion: 0.60
Nodes (4): install_group(), log(), provision.sh script, wait_for_apt()

### Community 200 - "gtkui_infobar_show"
Cohesion: 0.15
Nodes (7): is_pcap_file(), gtkui_conf_get(), gtkui_build_widgets(), gtkui_infobar_new(), gtkui_infobar_show(), gtkui_message_dialog(), read_pcapfile()

### Community 202 - "context when requiring the module"
Cohesion: 0.40
Nodes (5): before, it does not override any other module access points, it injects an enhanced format function, it returns the unextended module table, context when requiring the module

### Community 203 - "describe .. operator"
Cohesion: 0.40
Nodes (5): it concatenates string arguments, it stringifies nil arguments, it stringifies non-string arguments, the original subject is not perturbed, describe .. operator

### Community 204 - "describe string.numbertosi "
Cohesion: 0.40
Nodes (5): before, it coerces string arguments to a number, it diagnoses non-numeric arguments, it returns a number using SI suffixes, describe string.numbertosi ()

### Community 205 - "describe string.split "
Cohesion: 0.40
Nodes (5): before, it diagnoses non-string arguments, it makes a table of substrings delimitied by a separator, the original subject is not perturbed, describe string.split ()

### Community 206 - "describe table.empty "
Cohesion: 0.40
Nodes (5): before, it diagnoses non-table arguments, it returns false for a non-empty table, it returns true for an empty table, describe table.empty ()

### Community 207 - "describe table.size "
Cohesion: 0.40
Nodes (5): before, it counts no keys in an empty table, it counts the number of keys in a table, it diagnoses non-table arguments, describe table.size ()

### Community 208 - "describe table.sort "
Cohesion: 0.40
Nodes (5): before, it diagnoses non-table arguments, it returns the sorted table, it sorts elements in place, describe table.sort ()

### Community 212 - "mbox.lua"
Cohesion: 0.83
Nodes (3): headers(), message(), parse()

## Knowledge Gaps
- **145 isolated node(s):** `ns_t_a`, `ns_t_a6`, `ns_t_aaaa`, `ns_t_afsdb`, `ns_t_any` (+140 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 1078 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **72 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `DataDumper()` connect `dumper.lua` to `base.lua`?**
  _High betweenness centrality (0.054) - this node is a cross-community bridge._
- **Are the 146 inferred relationships involving `ip_addr_ntoa()` (e.g. with `dissector_bgp()` and `dissector_cvs()`) actually correct?**
  _`ip_addr_ntoa()` has 146 INFERRED edges - model-reasoned connections that need verification._
- **What connects `ns_t_a`, `ns_t_a6`, `ns_t_aaaa` to the rest of the system?**
  _145 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `__ns_type` be split into smaller, more focused modules?**
  _Cohesion score 0.03773584905660377 - nodes in this community are weakly interconnected._
- **Why does `tostring()` connect `base.lua` to `packet.lua`, `dumper.lua`, `getopt.lua`?**
  _High betweenness centrality (0.054) - this node is a cross-community bridge._
- **Are the 79 inferred relationships involving `ip_addr_cmp()` (e.g. with `conntrack_match()` and `conntrack_update()`) actually correct?**
  _`ip_addr_cmp()` has 79 INFERRED edges - model-reasoned connections that need verification._
- **Should `get_decoder` be split into smaller, more focused modules?**
  _Cohesion score 0.04365079365079365 - nodes in this community are weakly interconnected._