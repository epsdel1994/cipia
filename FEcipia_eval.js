/*
    This file is part of cipia.
    See [cipia](https://github.com/epsdel1994/cipia) for detail.

    Copyright 2017 T.Hironaka

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

var sync;
var Module = {
    onRuntimeInitialized: function(){
        FS.mkdir("/book");
        FS.mount(IDBFS, {}, "/book");
        FS.syncfs(true, function(err){
            self.postMessage({ kind: "initialized", });
        });
    },
};

var evaling = false;
var req = null;

self.onmessage = function(e){
    if(e.data.kind === "load_stop"){
    } else if(e.data.kind === "load"){
        Module.ccall("c_ems_load", "number", ["string"], [e.data.board]);
        self.postMessage({ kind: "loaded", });
    } else if(e.data.kind === "eval_stop"){
        evaling = false;
    } else if(e.data.kind === "eval"){
        evaling = true;
        var loop_eval = function(){
            var res = Module.ccall("c_ems_eval", "number", ["string"], [e.data.board]);
            if((res < -0.5) || (evaling === false)){
                self.postMessage({ kind: "evaled", });
                return;
            } else {
                self.postMessage({ kind: "evaling", progress: res });
                setTimeout(loop_eval, 0);
            }
        };
        loop_eval();
    } else if(e.data.kind === "search"){
        Module.ccall("c_ems_cache", "number", ["string"], [e.data.board]);
    } else if(e.data.kind === "book_load"){
        var res = Module.ccall("c_ems_book", "number", [], []);
        if(res < -0.5){
            self.postMessage({kind: "book_loaded", isSuccess: false});
        } else {
            var contents = FS.readFile("output.txt", {encoding: "utf8"});
            self.postMessage({ kind: "book_loaded", board: contents, isSuccess:true });
        }
    } else if(e.data.kind === "book_download"){
        if(e.data.url === "cipia:last"){
            self.postMessage({ kind: "book_downloaded", isSuccess: true, });
        } else {
            req = new XMLHttpRequest();
            req.open("GET", e.data.url, true);
            req.responseType = "arraybuffer";

            req.onprogress = function(e){
                self.postMessage({kind: "book_downloading", progress: e.loaded/e.total, });
            };
            req.onload = function(e){
                FS.writeFile("/book/input.dat", new Uint8Array(req.response), {encoding: "binary"});
                FS.syncfs(false, function(){
                    self.postMessage({ kind: "book_downloaded", isSuccess: true, });
                });
            };
            req.onerror = function(e){
                self.postMessage({ kind: "book_downloaded", isSuccess: false, });
            };
            req.onabort = function(e){
                self.postMessage({ kind: "book_downloaded", isSuccess: false, });
            };
            req.ontimeout = function(e){
                self.postMessage({ kind: "book_downloaded", isSuccess: false, });
            };

            req.send();
        }
    } else if(e.data.kind === "book_stop"){
        req.abort();
    }
}
