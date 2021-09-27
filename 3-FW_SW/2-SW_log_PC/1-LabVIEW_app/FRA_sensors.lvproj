<?xml version='1.0' encoding='UTF-8'?>
<Project Type="Project" LVVersion="13008000">
	<Item Name="My Computer" Type="My Computer">
		<Property Name="server.app.propertiesEnabled" Type="Bool">true</Property>
		<Property Name="server.control.propertiesEnabled" Type="Bool">true</Property>
		<Property Name="server.tcp.enabled" Type="Bool">false</Property>
		<Property Name="server.tcp.port" Type="Int">0</Property>
		<Property Name="server.tcp.serviceName" Type="Str">My Computer/VI Server</Property>
		<Property Name="server.tcp.serviceName.default" Type="Str">My Computer/VI Server</Property>
		<Property Name="server.vi.callsEnabled" Type="Bool">true</Property>
		<Property Name="server.vi.propertiesEnabled" Type="Bool">true</Property>
		<Property Name="specify.custom.address" Type="Bool">false</Property>
		<Item Name="00_FRA_sensors_main.vi" Type="VI" URL="../00_FRA_sensors_main.vi"/>
		<Item Name="01_FRA_Data_converter.vi" Type="VI" URL="../01_FRA_Data_converter.vi"/>
		<Item Name="Add to array if not in array.vi" Type="VI" URL="../subVIs/Add to array if not in array.vi"/>
		<Item Name="Add_row_to_string_indicator.vi" Type="VI" URL="../subVIs/Add_row_to_string_indicator.vi"/>
		<Item Name="Convert_calc_app-spec_preview_graphs.vi" Type="VI" URL="../subVIs/Convert_calc_app-spec_preview_graphs.vi"/>
		<Item Name="Converter_add_to_sum_array.vi" Type="VI" URL="../subVIs/Converter_add_to_sum_array.vi"/>
		<Item Name="Converter_app_spec_set.ctl" Type="VI" URL="../subVIs/Converter_app_spec_set.ctl"/>
		<Item Name="Converter_average_fom_sum_array.vi" Type="VI" URL="../subVIs/Converter_average_fom_sum_array.vi"/>
		<Item Name="Converter_baseline_avg_to_string_indicator.vi" Type="VI" URL="../subVIs/Converter_baseline_avg_to_string_indicator.vi"/>
		<Item Name="Converter_baseline_indicator.ctl" Type="VI" URL="../subVIs/Converter_baseline_indicator.ctl"/>
		<Item Name="Converter_calc_app-spec.vi" Type="VI" URL="../subVIs/Converter_calc_app-spec.vi"/>
		<Item Name="Converter_get_baseline_DBL_arr.vi" Type="VI" URL="../subVIs/Converter_get_baseline_DBL_arr.vi"/>
		<Item Name="Converter_get_baseline_str_arr.vi" Type="VI" URL="../subVIs/Converter_get_baseline_str_arr.vi"/>
		<Item Name="Converter_sample_is_in_selected_time.vi" Type="VI" URL="../subVIs/Converter_sample_is_in_selected_time.vi"/>
		<Item Name="Converter_search_input_unit_get_column.vi" Type="VI" URL="../subVIs/Converter_search_input_unit_get_column.vi"/>
		<Item Name="Create_data_log_file.vi" Type="VI" URL="../subVIs/Create_data_log_file.vi"/>
		<Item Name="Custom-preview-graph-override.vi" Type="VI" URL="../subVIs/Custom-preview-graph-override.vi"/>
		<Item Name="Data_conv_UI.ctl" Type="VI" URL="../subVIs/Data_conv_UI.ctl"/>
		<Item Name="error_codes.csv" Type="Document" URL="../error_codes.csv"/>
		<Item Name="FRA_UDP_get_time.vi" Type="VI" URL="../subVIs/FRA_UDP_get_time.vi"/>
		<Item Name="FRA_UDP_get_timestamp_msg_type.vi" Type="VI" URL="../subVIs/FRA_UDP_get_timestamp_msg_type.vi"/>
		<Item Name="FRA_UDP_send_ACK.vi" Type="VI" URL="../subVIs/FRA_UDP_send_ACK.vi"/>
		<Item Name="Global Status bar event.vi" Type="VI" URL="../subVIs/Global Status bar event.vi"/>
		<Item Name="loop error, restart vi.vi" Type="VI" URL="../subVIs/loop error, restart vi.vi"/>
		<Item Name="loop error, restart vi_err_cluster.vi" Type="VI" URL="../subVIs/loop error, restart vi_err_cluster.vi"/>
		<Item Name="loop error, restart vi_polymorphic.vi" Type="VI" URL="../subVIs/loop error, restart vi_polymorphic.vi"/>
		<Item Name="Merge_single_configuration.vi" Type="VI" URL="../subVIs/Merge_single_configuration.vi"/>
		<Item Name="Name plot legend from array.vi" Type="VI" URL="../subVIs/Name plot legend from array.vi"/>
		<Item Name="Power_ctrl_to_ioEXPSET.vi" Type="VI" URL="../subVIs/Power_ctrl_to_ioEXPSET.vi"/>
		<Item Name="Results_cluster.ctl" Type="VI" URL="../subVIs/Results_cluster.ctl"/>
		<Item Name="Send global status message 4S.vi" Type="VI" URL="../subVIs/Send global status message 4S.vi"/>
		<Item Name="Send_simple_command.vi" Type="VI" URL="../subVIs/Send_simple_command.vi"/>
		<Item Name="Sensor_data_signed_unsigned_conv.vi" Type="VI" URL="../subVIs/Sensor_data_signed_unsigned_conv.vi"/>
		<Item Name="Sensor_data_to_value.vi" Type="VI" URL="../subVIs/Sensor_data_to_value.vi"/>
		<Item Name="Sensor_data_to_value_step1_resolve_sign.vi" Type="VI" URL="../subVIs/Sensor_data_to_value_step1_resolve_sign.vi"/>
		<Item Name="Sensor_data_to_value_step2_convert.vi" Type="VI" URL="../subVIs/Sensor_data_to_value_step2_convert.vi"/>
		<Item Name="Sensor_power_ctrl.ctl" Type="VI" URL="../subVIs/Sensor_power_ctrl.ctl"/>
		<Item Name="Split number byte.vi" Type="VI" URL="../subVIs/Split number byte.vi"/>
		<Item Name="Status send Error cluster.vi" Type="VI" URL="../subVIs/Status send Error cluster.vi"/>
		<Item Name="Sum_array_for_avg_baseline.ctl" Type="VI" URL="../subVIs/Sum_array_for_avg_baseline.ctl"/>
		<Item Name="Test_baseline_subVIs.vi" Type="VI" URL="../subVIs/Test_baseline_subVIs.vi"/>
		<Item Name="Tester_Sensor_data_to_value.vi" Type="VI" URL="../subVIs/Tester_Sensor_data_to_value.vi"/>
		<Item Name="UDP_read_datagram.ctl" Type="VI" URL="../subVIs/UDP_read_datagram.ctl"/>
		<Item Name="UI_states.ctl" Type="VI" URL="../subVIs/UI_states.ctl"/>
		<Item Name="Unit_select_s_m.ctl" Type="VI" URL="../subVIs/Unit_select_s_m.ctl"/>
		<Item Name="Untitled 1.vi" Type="VI" URL="../subVIs/Untitled 1.vi"/>
		<Item Name="Write_data_to_file_time_sender_dtgn.vi" Type="VI" URL="../subVIs/Write_data_to_file_time_sender_dtgn.vi"/>
		<Item Name="Write_error_to_error_indicator.vi" Type="VI" URL="../subVIs/Write_error_to_error_indicator.vi"/>
		<Item Name="Write_sensor_power.vi" Type="VI" URL="../subVIs/Write_sensor_power.vi"/>
		<Item Name="Dependencies" Type="Dependencies">
			<Item Name="vi.lib" Type="Folder">
				<Item Name="BuildHelpPath.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/BuildHelpPath.vi"/>
				<Item Name="Check Special Tags.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/Check Special Tags.vi"/>
				<Item Name="Clear Errors.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/Clear Errors.vi"/>
				<Item Name="Close File+.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/file.llb/Close File+.vi"/>
				<Item Name="compatReadText.vi" Type="VI" URL="/&lt;vilib&gt;/_oldvers/_oldvers.llb/compatReadText.vi"/>
				<Item Name="Convert property node font to graphics font.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/Convert property node font to graphics font.vi"/>
				<Item Name="Details Display Dialog.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/Details Display Dialog.vi"/>
				<Item Name="DialogType.ctl" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/DialogType.ctl"/>
				<Item Name="DialogTypeEnum.ctl" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/DialogTypeEnum.ctl"/>
				<Item Name="Error Code Database.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/Error Code Database.vi"/>
				<Item Name="ErrWarn.ctl" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/ErrWarn.ctl"/>
				<Item Name="eventvkey.ctl" Type="VI" URL="/&lt;vilib&gt;/event_ctls.llb/eventvkey.ctl"/>
				<Item Name="ex_CorrectErrorChain.vi" Type="VI" URL="/&lt;vilib&gt;/express/express shared/ex_CorrectErrorChain.vi"/>
				<Item Name="Find First Error.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/Find First Error.vi"/>
				<Item Name="Find Tag.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/Find Tag.vi"/>
				<Item Name="Format Message String.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/Format Message String.vi"/>
				<Item Name="General Error Handler CORE.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/General Error Handler CORE.vi"/>
				<Item Name="General Error Handler.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/General Error Handler.vi"/>
				<Item Name="Get String Text Bounds.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/Get String Text Bounds.vi"/>
				<Item Name="Get Text Rect.vi" Type="VI" URL="/&lt;vilib&gt;/picture/picture.llb/Get Text Rect.vi"/>
				<Item Name="GetHelpDir.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/GetHelpDir.vi"/>
				<Item Name="GetRTHostConnectedProp.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/GetRTHostConnectedProp.vi"/>
				<Item Name="Longest Line Length in Pixels.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/Longest Line Length in Pixels.vi"/>
				<Item Name="LVBoundsTypeDef.ctl" Type="VI" URL="/&lt;vilib&gt;/Utility/miscctls.llb/LVBoundsTypeDef.ctl"/>
				<Item Name="NI_Gmath.lvlib" Type="Library" URL="/&lt;vilib&gt;/gmath/NI_Gmath.lvlib"/>
				<Item Name="Not Found Dialog.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/Not Found Dialog.vi"/>
				<Item Name="Open File+.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/file.llb/Open File+.vi"/>
				<Item Name="Read File+ (string).vi" Type="VI" URL="/&lt;vilib&gt;/Utility/file.llb/Read File+ (string).vi"/>
				<Item Name="Read From Spreadsheet File (DBL).vi" Type="VI" URL="/&lt;vilib&gt;/Utility/file.llb/Read From Spreadsheet File (DBL).vi"/>
				<Item Name="Read From Spreadsheet File (I64).vi" Type="VI" URL="/&lt;vilib&gt;/Utility/file.llb/Read From Spreadsheet File (I64).vi"/>
				<Item Name="Read From Spreadsheet File (string).vi" Type="VI" URL="/&lt;vilib&gt;/Utility/file.llb/Read From Spreadsheet File (string).vi"/>
				<Item Name="Read From Spreadsheet File.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/file.llb/Read From Spreadsheet File.vi"/>
				<Item Name="Read Lines From File.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/file.llb/Read Lines From File.vi"/>
				<Item Name="Search and Replace Pattern.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/Search and Replace Pattern.vi"/>
				<Item Name="Set Bold Text.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/Set Bold Text.vi"/>
				<Item Name="Set String Value.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/Set String Value.vi"/>
				<Item Name="Simple Error Handler.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/Simple Error Handler.vi"/>
				<Item Name="subFile Dialog.vi" Type="VI" URL="/&lt;vilib&gt;/express/express input/FileDialogBlock.llb/subFile Dialog.vi"/>
				<Item Name="TagReturnType.ctl" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/TagReturnType.ctl"/>
				<Item Name="Three Button Dialog CORE.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/Three Button Dialog CORE.vi"/>
				<Item Name="Three Button Dialog.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/Three Button Dialog.vi"/>
				<Item Name="Trim Whitespace.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/Trim Whitespace.vi"/>
				<Item Name="whitespace.ctl" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/whitespace.ctl"/>
			</Item>
			<Item Name="lvanlys.dll" Type="Document" URL="/&lt;resource&gt;/lvanlys.dll"/>
		</Item>
		<Item Name="Build Specifications" Type="Build">
			<Item Name="FRA_logger_191130" Type="EXE">
				<Property Name="App_copyErrors" Type="Bool">true</Property>
				<Property Name="App_INI_aliasGUID" Type="Str">{230494EF-83E0-47FC-A418-A11F9226F278}</Property>
				<Property Name="App_INI_GUID" Type="Str">{55200340-1AD0-47E1-B175-0720E9A6A238}</Property>
				<Property Name="App_serverConfig.httpPort" Type="Int">8002</Property>
				<Property Name="Bld_autoIncrement" Type="Bool">true</Property>
				<Property Name="Bld_buildCacheID" Type="Str">{3D5F951C-1FF9-4E51-BB95-41B0A0E692BA}</Property>
				<Property Name="Bld_buildSpecName" Type="Str">FRA_logger_191130</Property>
				<Property Name="Bld_excludeInlineSubVIs" Type="Bool">true</Property>
				<Property Name="Bld_excludeLibraryItems" Type="Bool">true</Property>
				<Property Name="Bld_excludePolymorphicVIs" Type="Bool">true</Property>
				<Property Name="Bld_localDestDir" Type="Path">../builds/NI_AB_PROJECTNAME/FRA_logger_191130</Property>
				<Property Name="Bld_localDestDirType" Type="Str">relativeToCommon</Property>
				<Property Name="Bld_modifyLibraryFile" Type="Bool">true</Property>
				<Property Name="Bld_previewCacheID" Type="Str">{B84C39F5-0374-4E5C-B683-A4C308B68D58}</Property>
				<Property Name="Bld_version.build" Type="Int">5</Property>
				<Property Name="Bld_version.major" Type="Int">1</Property>
				<Property Name="Destination[0].destName" Type="Str">FRA_logger.exe</Property>
				<Property Name="Destination[0].path" Type="Path">../builds/NI_AB_PROJECTNAME/FRA_logger_191130/FRA_logger.exe</Property>
				<Property Name="Destination[0].preserveHierarchy" Type="Bool">true</Property>
				<Property Name="Destination[0].type" Type="Str">App</Property>
				<Property Name="Destination[1].destName" Type="Str">Support Directory</Property>
				<Property Name="Destination[1].path" Type="Path">../builds/NI_AB_PROJECTNAME/FRA_logger_191130/data</Property>
				<Property Name="DestinationCount" Type="Int">2</Property>
				<Property Name="Source[0].itemID" Type="Str">{B27D5400-1617-4341-973C-F2B681F8429B}</Property>
				<Property Name="Source[0].type" Type="Str">Container</Property>
				<Property Name="Source[1].destinationIndex" Type="Int">0</Property>
				<Property Name="Source[1].itemID" Type="Ref">/My Computer/00_FRA_sensors_main.vi</Property>
				<Property Name="Source[1].sourceInclusion" Type="Str">TopLevel</Property>
				<Property Name="Source[1].type" Type="Str">VI</Property>
				<Property Name="Source[2].destinationIndex" Type="Int">0</Property>
				<Property Name="Source[2].itemID" Type="Ref">/My Computer/error_codes.csv</Property>
				<Property Name="Source[2].sourceInclusion" Type="Str">Include</Property>
				<Property Name="SourceCount" Type="Int">3</Property>
				<Property Name="TgtF_companyName" Type="Str">Electronics design solutions s.r.o.</Property>
				<Property Name="TgtF_fileDescription" Type="Str">FRA_logger_191130</Property>
				<Property Name="TgtF_internalName" Type="Str">FRA_logger_191130</Property>
				<Property Name="TgtF_legalCopyright" Type="Str">GNU GPLv2</Property>
				<Property Name="TgtF_productName" Type="Str">FRA_logger_191130</Property>
				<Property Name="TgtF_targetfileGUID" Type="Str">{87A3AA43-6AB5-437E-8204-FF8BCE572319}</Property>
				<Property Name="TgtF_targetfileName" Type="Str">FRA_logger.exe</Property>
			</Item>
			<Item Name="FRAlog_data_converter" Type="EXE">
				<Property Name="App_copyErrors" Type="Bool">true</Property>
				<Property Name="App_INI_aliasGUID" Type="Str">{9471C769-64FB-48FD-BA7C-30FD7043E1CB}</Property>
				<Property Name="App_INI_GUID" Type="Str">{BD388BBD-6163-4412-9068-A358A69D83C3}</Property>
				<Property Name="App_serverConfig.httpPort" Type="Int">8002</Property>
				<Property Name="Bld_autoIncrement" Type="Bool">true</Property>
				<Property Name="Bld_buildCacheID" Type="Str">{EA87A516-161F-475A-A7AE-70DA73F1116A}</Property>
				<Property Name="Bld_buildSpecDescription" Type="Str">Converer for data from LABVIEW log or SD card log to volts</Property>
				<Property Name="Bld_buildSpecName" Type="Str">FRAlog_data_converter</Property>
				<Property Name="Bld_excludeInlineSubVIs" Type="Bool">true</Property>
				<Property Name="Bld_excludeLibraryItems" Type="Bool">true</Property>
				<Property Name="Bld_excludePolymorphicVIs" Type="Bool">true</Property>
				<Property Name="Bld_localDestDir" Type="Path">../builds/NI_AB_PROJECTNAME/FRAlog_data_converter</Property>
				<Property Name="Bld_localDestDirType" Type="Str">relativeToCommon</Property>
				<Property Name="Bld_modifyLibraryFile" Type="Bool">true</Property>
				<Property Name="Bld_previewCacheID" Type="Str">{66162FFC-A73E-4F0C-BF92-BE1EEA871371}</Property>
				<Property Name="Bld_version.build" Type="Int">2</Property>
				<Property Name="Bld_version.major" Type="Int">1</Property>
				<Property Name="Destination[0].destName" Type="Str">FRA_converter.exe</Property>
				<Property Name="Destination[0].path" Type="Path">../builds/NI_AB_PROJECTNAME/FRAlog_data_converter/FRA_converter.exe</Property>
				<Property Name="Destination[0].preserveHierarchy" Type="Bool">true</Property>
				<Property Name="Destination[0].type" Type="Str">App</Property>
				<Property Name="Destination[1].destName" Type="Str">Support Directory</Property>
				<Property Name="Destination[1].path" Type="Path">../builds/NI_AB_PROJECTNAME/FRAlog_data_converter/data</Property>
				<Property Name="DestinationCount" Type="Int">2</Property>
				<Property Name="Source[0].itemID" Type="Str">{5088507F-8D19-4FC8-8366-B68CEC865B58}</Property>
				<Property Name="Source[0].type" Type="Str">Container</Property>
				<Property Name="Source[1].destinationIndex" Type="Int">0</Property>
				<Property Name="Source[1].itemID" Type="Ref">/My Computer/01_FRA_Data_converter.vi</Property>
				<Property Name="Source[1].sourceInclusion" Type="Str">TopLevel</Property>
				<Property Name="Source[1].type" Type="Str">VI</Property>
				<Property Name="SourceCount" Type="Int">2</Property>
				<Property Name="TgtF_companyName" Type="Str">Electronics design solution s.r.o.</Property>
				<Property Name="TgtF_fileDescription" Type="Str">FRAlog_data_converter</Property>
				<Property Name="TgtF_internalName" Type="Str">FRAlog_data_converter</Property>
				<Property Name="TgtF_legalCopyright" Type="Str">GNU GPLv2</Property>
				<Property Name="TgtF_productName" Type="Str">FRAlog_data_converter</Property>
				<Property Name="TgtF_targetfileGUID" Type="Str">{FC557F13-08A8-4068-B9C2-005E4179BC6C}</Property>
				<Property Name="TgtF_targetfileName" Type="Str">FRA_converter.exe</Property>
			</Item>
		</Item>
	</Item>
</Project>
