///////////////////////////////////////////
// Global variables
var table;
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

function add_to_table_if_does_not_exist(table_id, fieldname){
    //console.log(fieldname)
    var sn = "cb_" + slugify(fieldname);
    //console.log("fieldname= " + fieldname + " sn = " + sn)
    if (($("#"+sn).length) == 0)
    {
        $("#"+table_id).append('<input type="checkbox" name="cb' + sn +'" id="'+sn+'"><label for="'+sn+'">'+fieldname+'</label>').trigger('create');
    }
}

function server_message_handler(data){
    //dbg('server_message_handler() data = ' + data, true)
    try {

        //TODO - not sure why I have to parse twice
        JsonData = JSON.parse(data);
        //console.log("JsonData = " + JsonData);
        //console.log("JsonData.time = " + JsonData.time);
        var timestamp = new Date(JsonData.time);
        var row_data = [
            timestamp.toLocaleTimeString(),
            JsonData.name,
            JsonData.level,
            JsonData.line_no,
            JsonData.msg,
            JsonData.filename,
            JsonData.funcname,
            JsonData.hostname,
            JsonData.username,
            ];
        //console.log("row_data = " + row_data);
        table.row.add(row_data).draw();
        add_to_table_if_does_not_exist("cbfn", JsonData.name);
        add_to_table_if_does_not_exist("cb_hostname", JsonData.hostname);
        add_to_table_if_does_not_exist("cb_username", JsonData.username);

    } catch(e) {
        dbg('JSON.parse error: "' + e + '". JsonData = ' + JsonData);
        return;
    }
}

$(document).ready(function() {

    open_websocket('192.168.1.12:8889', "log");
    table = $('#example').DataTable({
        //"order": [[ 0, "desc" ]],
        "paging":   false,
        "info":     true,
        "scrollY": "600px",
        "scrollX": true,
         "columns": [
            { "title": "Time" },
            { "title": "Loger" },
            { "title": "Level" },
            { "title": "Line#" },
            { "title": "Msg" },
            { "title": "Filename", "visible": false },
            { "title": "Funcname", "visible": false },
            { "title": "Hostname", "visible": false  },
            { "title": "Username", "visible": false  },
        ],
         "columnDefs": [
             { "width": "5%", "targets": 2 },
             { "width": "2%", "targets": 3 },
             { "width": "60%", "targets": 4 }
             ]
    });

    var dataSet = [['12:00:00','self','1',144,"Test",'ulog.js',"var dataSet","localhost",'root'],];

    // http://jsfiddle.net/KPkJn/9/
    var sn = 0;
    $( "#button_test" ).click(function() {
    $("#cbfn").after('<input type="checkbox" name="cb' + sn +'" id="cb'+sn+'"><label for="cb'+sn+'">SN '+sn+'</label>').trigger('create');
    sn++;
 });
});