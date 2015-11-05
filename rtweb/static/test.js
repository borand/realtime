///////////////////////////////////////////
// Global variables
var table;
var JsonData;
//var JsonData = {"time": "04-Nov-2015 16:01:33","name": "test_a","level": 1,"line_no": 0,"filename": "none","funcname": "none","msg": "Test","hostname": "onw-aborowie-01","username": "aborowie"};

/////////////////////////////////////////////////////////////////////
// UTILITY FUNCTIONS
//
/////////////////////////////////////////////////////////////////////
// UTILITY FUNCTIONS
//
function dbg(message, show) {
    if (show){
        console.log(message);
    }
}

///////////////////////////////////////////////////////////////////////
// WEBSOCKETS FUNCTIONS
//MessageHandler
//
function slugify(text)
{
  return text.toString().toLowerCase()
    .replace(/\s+/g, '-')           // Replace spaces with -
    .replace(/[^\w\-]+/g, '')       // Remove all non-word chars
    .replace(/\-\-+/g, '-')         // Replace multiple - with single -
    .replace(/^-+/, '')             // Trim - from start of text
    .replace(/-+$/, '');            // Trim - from end of text
}

function is_log_active(JsonData, fieldname){
    var tmp = [];
    var x = $("input[name='cb_"+fieldname+"']");
    var sn = "cb_" + slugify(JsonData[fieldname]);
    if (x.length){
        //$("input[name='cbcb_filename']").each(function() {if($(this).is(":checked")){tmp.push($(this).attr('id'));}});        
        x.each(function() {if($(this).is(":checked")){tmp.push($(this).attr('id'));}});        
        return tmp.indexOf(sn) > -1;
    }
    else{
        return true
    };
};

function console_response_msg(message, show) {
    if(show){
        dbg(message);        
        $("#ws_console").html($("#ws_console").text() + message + '\n');
        var psconsole = $('#ws_console');
        psconsole.scrollTop(psconsole[0].scrollHeight - psconsole.height());
    }
};

function open_websocket(hostname, hosturl) {
    dbg('Attempting to open web socket',true);

    var websocket_address = "ws://" + hostname + "/websocket/" + hosturl;
    ws = new WebSocket(websocket_address);

    ws.onopen = function() {
        dbg('web socket open', true);
        $('#live').text('CONNECTED');
        $("#live").css("background-color",'#B2BB1E');
    };

    ws.onmessage = function(event) {
        //dbg('incomming message', true);
        server_message_handler(event.data);
    };
    ws.onclose = function() {
        dbg('closing websockets', true);
        $('#live').text('OFFLINE');
        $("#live").css("background-color",'#FF0000');
    };
}

function add_to_table_if_does_not_exist(Obj, fieldname){
    //console.log(fieldname)    
    var sn = "cb_" + slugify(Obj[fieldname]);
    var table_id = 'cb_' + fieldname; 
    //console.log("fieldname= " + fieldname + " sn = " + sn)
    if (($("#"+sn).length) == 0)
    {
        $("#"+table_id).append('<input type="checkbox" data-mini="true" checked=true name="' + table_id +'" id="'+sn+'"><label for="'+sn+'">'+Obj[fieldname]+'</label>').trigger('create');
    }
}

function server_message_handler(data){
    //dbg('server_message_handler() data = ' + data, true)
    try {

        if($("#flip_show_raw").val() === 'on'){
            console_response_msg(data, true);
        };
        JsonData = JSON.parse(data);
        //console.log("JsonData = " + JsonData);
        //console.log("JsonData.time = " + JsonData.time);
        //var x = $("input[name='cbcb_filename']");


        var timestamp = new Date(JsonData.time);
        // Convert level to numerical value
        // We redefine python key value to our own standard, the higher the number the more debug info
        var level_py = {"CRITICAL":0, "ERROR":0,"WARNING":1,"INFO":1,"DEBUG":3}
        if (typeof JsonData.level === 'string' || JsonData.level instanceof String){
             JsonData.level = level_py[JsonData.level.toUpperCase()];
        };

        var row_data = [
            timestamp.toLocaleTimeString(),
            JsonData.name,
            JsonData.level,
            JsonData.line_no,
            JsonData.filename,
            JsonData.funcname,
            JsonData.hostname,
            JsonData.username,
            JsonData.msg,
            ];



        add_to_table_if_does_not_exist(JsonData,'name');
        add_to_table_if_does_not_exist(JsonData,'hostname');
        add_to_table_if_does_not_exist(JsonData,'username');
        add_to_table_if_does_not_exist(JsonData,'filename');

        //table.row.add(row_data).draw();
        
        if (is_log_active(JsonData,'name')     && is_log_active(JsonData,'hostname') &&
            is_log_active(JsonData,'username') && is_log_active(JsonData,'filename') &&
            parseInt($("#max_log_level").val()) >= JsonData.level){
             table.row.add(row_data).draw();
        };
        

    } catch(e) {
        dbg('JSON.parse error: "' + e + '". JsonData = ' + JsonData);
        console_response_msg(JsonData, true);
        return;
    }
}

function connect_to_websocket_host(){
    var hostname = $('#hostname').val();
    var hostport = $('#hostport').val();
    var hosturl  = $('#hosturl').val();
    dbg('Pressed button: button_connect: [host, port] ' + hostname +':' + hostport + '/websocket/'+ hosturl, true);
    open_websocket(hostname+':'+hostport, hosturl);
}
$(document).ready(function() {
    
    connect_to_websocket_host();
    table = $('#example').DataTable({
        "order": [[ 0, "desc" ]],
        "dom": '<"top"if>t<"bottom"lp><"clear">',
        "paging":   false,
        "info":     true,        
        "scrollY": "600px",
        "bInfo": false,
        "scrollX": true,        
         "columns": [
            { "title": "Time"    ,"width":"10%" },
            { "title": "Loger"   ,"width":"10%" },
            { "title": "Level"   ,"width":"5%"  },
            { "title": "Line#"   ,"width":"5%"  },
            { "title": "Filename","width":"10%", "visible": false },
            { "title": "Funcname","width":"10%", "visible": false },
            { "title": "Hostname","width":"10%", "visible": false  },
            { "title": "Username","width":"10%", "visible": false  },
            { "title": "Msg" }
        ]
    });

    $('a.toggle-vis').on( 'click', function (e) {
        console.log(this);
        e.preventDefault();
 
        // Get the column API object
        var column = table.column( $(this).attr('data-column') );
 
        // Toggle the visibility
        column.visible( ! column.visible() );
        table.columns.adjust().draw();
    } );

    $("#button_connect").click(function() {  connect_to_websocket_host();   });
    
    // http://jsfiddle.net/KPkJn/9/    
    $( "#button_test" ).click(function() {/* */});

    $( "#button_clear" ).click(function(){ table.clear().draw(); });
    $( "#button_clear_console" ).click(function(){ $("#ws_console").html("");});

});