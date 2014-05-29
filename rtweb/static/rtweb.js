///////////////////////////////////////////
// Global variables
var active_tab;
var debug_websocket = false;
var debug_js = true;
var debug_all = true;

////////////////////////////////////////
// Chart and plot variables
var plot_height = 450;
var chart;
var plot;
/////////////////////////////////////////////////////////////////////
// UTILITY FUNCTIONS
//
//
function dbg(message) {
	console.log(message);
	show_server_msg(message);	
}

function SendCmd(cmd, val) {
	return $.getJSON('/cmd/', "cmd=" + cmd + "&param=" + val, function(data) {			
		$("#cmd_status").text(data.cmd);
	});
}

function show_server_msg(message) {
	if (debug_all)
	{	
		$("#debug_console").html( $("#debug_console").text() + message + '\n');					
	    var psconsole = $('#debug_console');
	    psconsole.scrollTop(psconsole[0].scrollHeight - psconsole.height());
	}
}

function console_response_msg(message) {	
	$("#json_res").html($("#json_res").text() + "cmd [" + message[1] + "]: " + message[2].data + '\n');
	var psconsole = $('#json_res');
	psconsole.scrollTop(psconsole[0].scrollHeight - psconsole.height());
}

function set_object_value(id, val){
	var datarole = $("#"+id).attr('data-role');
	dbg('id:' + id + " data-role: " + datarole + "  val: " + val);
	switch(datarole){
		case 'slider':
			dbg('case: slider');
			$('#' + id).val(val).slider("refresh");
			break;
		case 'flipswitch':			
			dbg('about to flip the switch value to:' + val + ' currently set to: ' + $('#' + id).val());
			$('#' + id).val(val).flipswitch("refresh");
			break;
		case 'text':
			$('#' + id).text(val);
			break
		default:
			dbg('case: default');
			$('#' + id).val(val)[datarole]("refresh");
	}
}

///////////////////////////////////////////////////////////////////////
// HIGHCHARTS
//
//
function empty_data() {
	var data = [], time = (new Date()).getTime(), i;
	for( i = -999; i <= 0; i++) {
		data.push([
			time + i * 1000,
			0.0
			]);
	}
	return data;
};

function draw_chart(render_to) {
	
	Highcharts.setOptions({
		global : {
			useUTC : false
		}
	});
	
	// Create the chart
	chart = new Highcharts.StockChart({
		chart : {
			renderTo : render_to,
			height : plot_height,			
		},
		
		rangeSelector: {
			buttons: [{
				count: 1,
				type: 'minute',
				text: '1M'
			}, {
				count: 5,
				type: 'minute',
				text: '5M'
			}, {
				type: 'all',
				text: 'All'
			}],
			inputEnabled: true,
			selected: 0
		},		
		title : {
			text : 'Timeseries Data'
		},
		
		exporting: {
			enabled: false
		},
		
		legend : {
			enabled: true
		},
		
		yAxis : {
			title : {
				text : 'VAL'
			},
			//max : 100,
			//min : 0,
		},
		
		series : [{
			name : 'Data 0',
			color: '#00FF00',
			step: true,
			data : empty_data(),			
		}]
	});
}

function draw_plot() {
	plot = new Highcharts.Chart({
		chart : {
			renderTo : 'plot2',
			defaultSeriesType : 'scatter',
			zoomType : 'xy'
		},
		title : {
			text : 'X-POL'
		},
		subtitle : {
			text : ' '
		},
		xAxis : {
			title : {
				enabled : true,
				text : 'I'
			},
			startOnTick : true,
			endOnTick : true,
			showLastLabel : true
		},
		yAxis : {
			title : {
				text : 'Q'
			}			
		},
		tooltip : {
			formatter : function() {
				return '' + this.x + ' ' + this.y + ' ';
			}
		},
		plotOptions : {
			scatter : {
				marker : {
					radius : 2,
					states : {
						hover : {
							enabled : true,
							lineColor : 'rgb(100,100,100)'
						}
					}
				},
				states : {
					hover : {
						marker : {
							enabled : false
						}
					}
				}
			}
		},
		series : [{
			name : 'X',
			color : 'rgba(223, 83, 83, .5)',
			data : [[0, 1]]

		}, {
			name : 'Y',
			color : 'rgba(119, 152, 191, .5)',
			data : [[1, 0]]

		}]
	});
}


function add_measurement(value){
	var t = (new Date()).getTime();
	var num_of_series = chart.series.length;
	
	console.log('num_of_series = ' + num_of_series)-1;	
	
	for (i=0;i<value.length;i++){
		if (i >= num_of_series-1){
			chart.addSeries({name : 'Data[' + i + ']', data : empty_data()},false,false);
		}
		else{
			series = chart.series[i];		
			console.log('series.name = '+ series.name +', value[' + i +'] = ' + value[i]);
			if(series.name != 'Navigator'){
				series.addPoint([t, value[i]], true, true);	
			}
		}
	}
}

function parse_message(message_text){
	var temp;
}
///////////////////////////////////////////////////////////////////////
// WEBSOCKETS FUNCTIONS
//
//
function open_websocket(hostname, hostport, hosturl) {

	dbg('Attempting to open web socket');
	function show_message(message) {
		show_server_msg(message);		
	}

	var websocket_address = "ws://" + hostname + ":" + hostport + "/" + hosturl;
	var ws = new WebSocket(websocket_address);
	
	ws.onopen = function() {
		dbg('web socket open');
		$('#live').text('CONNECTED');
		$("#live").css("background-color",'#B2BB1E');
	};

	ws.onmessage = function(event) {
		dbg('incomming message');
		server_message_handler(event.data);
	};
	ws.onclose = function() {
		dbg('closing websockets');
		$('#live').text('OFFLINE');
	};
}

function server_message_handler(data)
{
	var JsonData;

	try {
		JsonData = JSON.parse(data);
	} catch(e) {
		dbg('JSON.parse error: "' + e + '". JsonData = ' + JsonData);
		return;

	}
	if (JsonData.hasOwnProperty('id')) {
		console.log(JsonData.id);
		switch(JsonData.id)
		{
			case 'debug_console':
			{
				console_response_msg(JsonData.data);
				console.log(JsonData.data[2].cmd === 'irq_0');
				if (JsonData.data[2].cmd === 'irq_0'){
					var msg = JsonData.data[2].data;
					console.log(msg);
					power_W = Math.round(3600.0/((Math.pow(2,16)*msg[2] + msg[3])/16e6*1024));						
					console.log([power_W]);
					add_measurement([power_W]);
				}
				break;
			}
			case 'chart':
			{
				add_measurement(power_W);
				break;
			}
			default:
			{	
				set_object_value(JsonData.id,JsonData.val);
			}
		}

	}
}

function connect_to_websocket_host(){
	var hostname = $('#hostname').val();
	var hostport = $('#hostport').val();
	var hosturl  = $('#hosturl').val();
	dbg('Pressed button: button_connect: [host, port] ' + hostname +':' + hostport + '/'+ hosturl);
	open_websocket(hostname, hostport, hosturl);
}
///////////////////////////////////////////////////////////////////////
// MAIN GUI - jQUERY
//
//
$(document).ready(function() {

	dbg('Document ready');

	debug_websocket = $('#debug_websocket').prop("checked");
	debug_js        = $('#debug_js').prop("checked");
	debug_all       = $('#debug_all').prop("checked");
	
	$( "#radio-websocket-online" ).prop( "checked", false ).checkboxradio( "refresh" );
	
	$('#json_res').attr('style', 'background-color:White; font-size:14px; height: 20em;');
	$('#json_res').textinput("option", "autogrow", false);
	//$('#launch_power').​​​attr('style', 'background-color:White; font-size:14px; width: 5em;');

	$('#debug_console').attr('style', 'background-color:White; font-size:14px; height: 20em;');
	$('#debug_console').textinput("option", "autogrow", false);
		
	$('#server_msg').textinput("option", "autogrow", false);
	$("#live").css("background-color",'#C71C2C');
	
	draw_chart('chart');
	draw_plot();
	connect_to_websocket_host();
	
	///////////////////////////////////////////////////////////////////////
	$('#json_cmd').keydown(function(e) {
		if (e.keyCode == 13) {
			var cmd = $("#json_cmd").val();
			$(this).val("");
			if (cmd == "clc") {
				console.log('Clear screen');
				$("#json_res").text("");
			} else {
				if (cmd == '') {
					console.log('Sending empty command');
					cmd = ' ';
				} else {
					console.log('Sending command: ' + cmd);
				}

				$("#json_res").append("cmd>" + cmd + "\n");

				$.getJSON('/cmd/', "cmd=" + cmd, function(data) {
					//console.log(String(data));
					$("#json_res").html($("#json_res").text() + data.res + '\n');					
					var psconsole = $('#json_res');
					psconsole.scrollTop(psconsole[0].scrollHeight - psconsole.height());
				});
			}
		}
	});
	///////////////////////////////////////////////////////////////////////
	$('#js_eval').keydown(function(e) 
	{
		if (e.keyCode == 13) 
		{
			var cmd = $("#js_eval").val();
			dbg('eval(' + cmd + ')');
			eval(cmd);
			$("#js_eval").val('');
		}
	});
	///////////////////////////////////////////////////////////////////////
	//
	// BUTTONS
	//
	$(".custom").change(function() {	
		debug_all = $( "#debug_all" ).prop("checked");
		dbg("debug_all = " + debug_all);
		//alert( "Handler for .change() called." );
		//$("#debug_all").prop("checked", true).checkboxradio("refresh");
	});

	$("#button_connect").click(function() {	
		connect_to_websocket_host();
	});

	$("#button_clear_debug_console").click(function() {
		$("#debug_console").text("");
	});

	$("#options_ping").click(function() {		
		SendCmd('ping', 0);
		$("#cmd_status").text("Pressed options_ping button");
	});


	$("#button_power_up").click(function() {
		cmd = '10';
		// var t = (new Date()).getTime();
		// chart.update({
			// type : 'flags',
				// data : [{
					// x : t,
					// title : 'P',					
				// }]});
        
     
		// var cssObj = {			
			// 'color' : 'rgb(1,0,0)'
		// }
		 // $('#launch_power').css(cssObj);


		if ($('#power_control_enabled').prop("checked")) {
			$.getJSON('/cmd/', "cmd=" + cmd, function(data) {
				//console.log(String(data));
				$("#json_res").html($("#json_res").text() + data.res + '\n');
				var psconsole = $('#json_res');
				psconsole.scrollTop(psconsole[0].scrollHeight - psconsole.height());
			});
		};
		});

	
	$("#button_power_down").click(function() {
		cmd = '11';			
		if ($('#power_control_enabled').prop("checked")) {
			dbg("Power Down");
			$.getJSON('/cmd/', "cmd=" + cmd, function(data) {
				//console.log(String(data));
				$("#json_res").html($("#json_res").text() + data.res + '\n');
				var psconsole = $('#json_res');
				psconsole.scrollTop(psconsole[0].scrollHeight - psconsole.height());
			});
		}
		else{
			dbg("Power control disabled");
		}
		});




});
