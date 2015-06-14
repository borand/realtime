///////////////////////////////////////////
// Global variables
var active_tab;
var debug_websocket = false;
var debug_js = true;
var debug_all = true;

var ws;
////////////////////////////////////////
// Chart and plot variables
var plot_height = 450;
var gauge;
/////////////////////////////////////////////////////////////////////
// UTILITY FUNCTIONS
//
//
function dbg(message, show) {	
	show_server_msg(message, show);	
}

function SendCmd(cmd, val) {
	return $.getJSON('/cmd/', "cmd=" + cmd + "&param=" + val, function(data) {			
		$("#cmd_status").text(data.cmd);
	});
}

function show_server_msg(message, show) {	
	if (show)
	{	
		console.log(message);		
	}
}

function console_response_msg(message, show) {
	if(show){
		dbg(message,true);
		chan = message['FROM'];
		console.log(message['MSG'])
		//$("#json_res").html($("#json_res").text() + chan + "> " + "cmd [" + message['MSG'][1] + "]: " + message['MSG'][2].data + '\n');
		$("#json_res").html($("#json_res").text() + chan + "> " + JSON.stringify(message['MSG']) + '\n');
		var psconsole = $('#json_res');
		psconsole.scrollTop(psconsole[0].scrollHeight - psconsole.height());
	}
}

function set_object_value(id, val){
	var datarole = $("#"+id).attr('data-role');
	dbg('id:' + id + " data-role: " + datarole + "  val: " + val, true);
	switch(datarole){
		case 'slider':
			dbg('case: slider', true);
			$('#' + id).val(val).slider("refresh");
			break;
		case 'flipswitch':			
			dbg('about to flip the switch value to:' + val + ' currently set to: ' + $('#' + id).val(), true);
			$('#' + id).val(val).flipswitch("refresh");
			break;
		case 'text':
			$('#' + id).text(val);
			break
		default:
			dbg('case: default', true);
			$('#' + id).val(val)[datarole]("refresh");
	}
}

///////////////////////////////////////////////////////////////////////
// HIGHCHARTS
//
//

function draw_plot() {
	dbg('draw_plot', true);
	
	gauge = new Highcharts.Chart({
		 chart: {
		 	renderTo: 'hydro',
            type: 'solidgauge'
        },

        title: null,

        pane: {
            center: ['50%', '85%'],
            size: '140%',
            startAngle: -90,
            endAngle: 90,
            background: {
                backgroundColor: (Highcharts.theme && Highcharts.theme.background2) || '#EEE',
                innerRadius: '60%',
                outerRadius: '100%',
                shape: 'arc'
            }
        },

        tooltip: {
            enabled: false
        },

        // the value axis
        yAxis: {
            stops: [
                [0.1, '#55BF3B'], // green
                [0.5, '#DDDF0D'], // yellow
                [0.9, '#DF5353'] // red
            ],
            lineWidth: 0,
            minorTickInterval: null,
            tickPixelInterval: 400,
            tickWidth: 0,
            title: {
                y: -70
            },
            labels: {
                y: 16
            },
            min: 0,
            max: 10000,
            title: {
                text: 'Power'
            }
        },

        plotOptions: {
            solidgauge: {
                dataLabels: {
                    y: 5,
                    borderWidth: 0,
                    useHTML: true
                }
            }
        },
        
        series: [{
            name: 'Power',
            data: [6000],
            dataLabels: {
                format: '<div style="text-align:center"><span style="font-size:25px;color:' +
                    ((Highcharts.theme && Highcharts.theme.contrastTextColor) || 'black') + '">{y}</span><br/>' +
                       '<span style="font-size:12px;color:silver">km/h</span></div>'
            },
            tooltip: {
                valueSuffix: 'W'
            }
        }]
	});
}

function add_measurement(value){
	
	var t = (new Date()).getTime();
	var num_of_series = chart.series.length;
	
	console.log('adding value to the plot :' + value);	

	for (i=0;i<value.length;i++){
		if (i >= num_of_series-1){
			chart.addSeries({name : 'Data[' + i + ']', data : empty_data()},false,false);
		}
		else{
			series = chart.series[i];		
			dbg('series.name = '+ series.name +', value[' + i +'] = ' + value[i],$('#debug_chart').prop("checked"));
			if(series.name != 'Navigator'){
				series.addPoint([t, value[i]], true, true);	
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////
// WEBSOCKETS FUNCTIONS
//
//
function open_websocket(hostname, hostport, hosturl) {

	dbg('Attempting to open web socket',true);
	function show_message(message) {
		show_server_msg(message);		
	}

	var websocket_address = "ws://" + hostname + ":" + hostport + "/websocket/" + hosturl;
	ws = new WebSocket(websocket_address);
	
	ws.onopen = function() {
		debug_websocket = $('#debug_websocket').prop("checked");
		dbg('web socket open', debug_websocket);
		$('#live').text('CONNECTED');
		$("#live").css("background-color",'#B2BB1E');
	};

	ws.onmessage = function(event) {
		debug_websocket = $('#debug_websocket').prop("checked");
		dbg('incomming message', debug_websocket);
		server_message_handler(event.data);
	};
	ws.onclose = function() {
		debug_websocket = $('#debug_websocket').prop("checked");
		dbg('closing websockets', debug_websocket);
		$('#live').text('OFFLINE');
		$("#live").css("background-color",'#FF0000');
	};
}

function server_message_handler(data){
	var JsonData;

	try {
		JsonData = JSON.parse(data);
	} catch(e) {
		dbg('JSON.parse error: "' + e + '". JsonData = ' + JsonData);
		return;

	}
	//console.log(JsonData)
	console_response_msg(JsonData, true);
	
	var sn_to_plot = $('#sn_to_plot').val();
	if (JsonData.hasOwnProperty('MSG')) {
				
				if (JsonData.MSG.hasOwnProperty('sn')){
					if (JsonData.MSG.sn === sn_to_plot){
						data = JsonData.MSG.data[0];
						console.log('adding value to the plot :' + data)
						add_measurement([data]);
					};
				};
				if (JsonData.MSG.cmd === 'irq_0'){
					//dbg(JsonData.data, $('#debug_irq').prop("checked"))
					//msg = JsonData.data[2].data;					
					//console_response_msg(JsonData.data, $('#debug_irq').prop("checked"));
					data = JsonData.MSG.data[1];
					power_W = Math.round(3600.0/((Math.pow(2,16)*data[1] + data[2])/16e6*1024));
					console.log(power_W);
					add_measurement([power_W]);
				}
	}
}

function connect_to_websocket_host(){
	var hostname = 'localhost';
	var hostport = 8000;
	var hosturl  = 'rtweb';
	dbg('Pressed button: button_connect: [host, port] ' + hostname +':' + hostport + '/websocket/'+ hosturl, true);
	open_websocket(hostname, hostport, hosturl);
}
///////////////////////////////////////////////////////////////////////
// MAIN GUI - jQUERY
//
//
$(document).ready(function() {

	dbg('Document ready', true);
	$("#live").css("background-color",'#C71C2C');	
	connect_to_websocket_host();
	draw_plot();

 //    // The speed gauge
 //    $('#hydro').highcharts(Highcharts.merge(gaugeOptions, {

 //    }));
    
	////////////////////////////////////
});
