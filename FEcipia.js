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

"use strict";

var FEcipia = {};

FEcipia.setup = function(fwcyan){

    fwcyan.setColor(240);

    var og = fwcyan.plugin.JSprinCore.ReversiGame.create();
    og.change = function(){
        undo.setScore((this._enabled === false) ? 0
            : (this.canundo()===true) ? 1 : 0);
        redo.setScore((this._enabled === false) ? 0
            : (this.canredo()===true) ? 1 : 0);
        edit.setScore(this._enabled ? 1 : 0);
        if(dl.isProcessing() === false){
            dl.setScore(this._enabled ? 1 : 0);
        }
        if(ceval.isProcessing() === false){
            ceval.setScore((this.isGameOver() === true) ? 0 : 1);
        }
        cstatus.update(this._turn, this._board);
        if((this.isGameOver() === false)
            && (eval_loaded === true)
            && (this.getEnabled() === true)
            && (this._board._valueMaxCell === null)
            ){
            eval_worker.onmessage = function(e){
                var lower_sign = "";
                var upper_sign = "";
                if(e.data.lower > 0){ lower_sign = "+"; };
                if(e.data.upper > 0){ upper_sign = "+"; };
                res.setString(lower_sign + e.data.lower + "/" + upper_sign + e.data.upper);
                og.setMaxCell(e.data.x, e.data.y);
                og.setEnabled(true);
            };
            eval_worker.postMessage(
                {"kind": "search", "board": og.getBoard()}
            );
            og.setEnabled(false);
        } else if ( this._board._valueMaxCell === null ) {
            res.setString("--/--");
        } else {
        }
    };

    var undo = fwcyan.Button.create("<", 1);
    undo.select = function(){ og.undo(); };
    var redo = fwcyan.Button.create(">", 1);
    redo.select = function(){ og.redo(); };

    var hist = [];

    var edit = fwcyan.Button.create("Edit", 1);
    edit.select = function(){
        var board = og.getBoard();
        ob.setBoard(board.slice(0,64));
        if(board.charAt(65) === "X"){
            toggle_turn.setItem(turnToggle_Black);
        } else if(board.charAt(65) === "O"){
            toggle_turn.setItem(turnToggle_White);
        } else if(board.charAt(65) === "-"){
            toggle_turn.setItem(turnToggle_Black);
        }
        toggle_put.setItem(putToggle_Black);
        popup_edit.open();
    };

    var eval_worker = null;
    var eval_loaded = false;
    var ceval = fwcyan.ButtonProgress.create( "Evaluate", "Evaluating", 1);
    ceval.start = function(){
        if(eval_worker !== null){
            og.setEnabled(false);
            eval_worker.onmessage = function(e){
                if(e.data.kind === "evaled"){
                    ceval.finish();
                    og._board._resetCellValues();
                    eval_loaded = true;
                    ceval.setString("Evaluate");
                    og.setEnabled(true);
                } else if(e.data.kind === "evaling"){
                    ceval.progress(e.data["progress"]);
                } else {
                    console.log("ERROR:", e);
                }
            };
            eval_worker.postMessage({
                "kind": "eval", "board": og.getBoard(),
            });
            ceval.stop = function(){
                eval_worker.postMessage({
                    "kind": "eval_stop",
                });
            };
        } else {
            ceval.setString("Preparing");
            ceval.setScore(0);
            ceval.progress(false);
            eval_worker = new Worker("FEcipia_eval.js");
            eval_worker.onmessage = function(e){
                if(e.data.kind === "initialized"){
                    eval_worker.postMessage({
                        "kind": "load", "board": og.getBoard(),
                    });
                } else if(e.data.kind === "loaded"){
                    ceval.finish();
                    ceval.setString("Evaluate");
                    edit.setScore(1);
                } else { ceval.stop(); }
            };
            edit.setScore(0);
            ceval.stop = function(){
                eval_worker.terminate();
                ceval.finish();
                eval_worker = null;
                edit.setScore(1);
            };
        }
    };

    var cstatus = fwcyan.plugin.JSprinCore.ReversiStatus.create();

    var about = fwcyan.Button.create("About", 1);
    about.select = function(){
        window.open("https://github.com/epsdel1994/cipia");
    };

    var res = fwcyan.Button.create("--/--", 0);
    var dl = fwcyan.ButtonProgress.create("Book import", "Downloading", 1);
    dl.start = function(){
        var str = prompt("Input book URL to load.\n(\"cipia:last\" to load the book downloaded last time.)");
        if((str !== null) && (str !== "")){
            var eval_worker_2 = new Worker("FEcipia_eval.js");
            eval_worker_2.onmessage = function(e){
                if(e.data.kind === "initialized"){
                    eval_worker_2.postMessage({
                        "kind": "book_download", "url": str,
                    });
                } else if(e.data.kind === "book_downloaded"){
                    if(e.data["isSuccess"] === true){
                        dl.setString("Preparing");
                        dl.progress(false);
                        eval_worker_2.postMessage({
                            "kind": "book_load",
                        });
                    } else {
                        alert("Failed to download book file.");
                        eval_worker_2.terminate();
                        og.setEnabled(true);
                        dl.finish();
                    }
                } else if(e.data.kind === "book_loaded"){
                    if(e.data["isSuccess"] === true){
                        og.init(e.data["board"]);
                        if(eval_worker !== null){
                            eval_worker.terminate();
                        } 
                        eval_worker = eval_worker_2;
                        eval_loaded = true;

                        og.setEnabled(true);
                        dl.finish();
                    } else {
                        alert("Failed to read book file.");
                        eval_worker_2.terminate();
                        og.setEnabled(true);
                        dl.finish();
                    }
                } else if(e.data.kind === "book_downloading"){
                    dl.progress(e.data["progress"]);
                }
            };
            og.setEnabled(false);
            ceval.setScore(0);
            dl.stop = function(){
                eval_worker_2.terminate();
                og.setEnabled(true);
                dl.finish();
            };
        } else {
            dl.finish();
        }
    };

    var main = fwcyan.Popup.create(fwcyan.Template1.create([
        og, undo, redo,
        dl,
        res,
        edit,
        ceval,
        cstatus,
        about,
    ]));
    fwcyan.setMain(main);

    var ob = fwcyan.plugin.JSprinCore.ReversiBoard.create();
    ob.pointCell = function(e){
        this.setCell(e.x, e.y, putColor);
        this._update();
    };
    ob.movepointCell = function(e){
        this.setCell(e.x, e.y, putColor);
        this._update();
    };

    var putColor = ob.color.Black;
    var turnColor = ob.color.Black;

    var putToggle_Black = 
        fwcyan.plugin.JSprinCore.ReversiDisk.create(ob.color.Black);
    var putToggle_White = 
        fwcyan.plugin.JSprinCore.ReversiDisk.create(ob.color.White);
    var putToggle_Empty = 
        fwcyan.plugin.JSprinCore.ReversiDisk.create(ob.color.Empty);
    var putToggle_None = 
        fwcyan.plugin.JSprinCore.ReversiDisk.create(ob.color.None);
    var turnToggle_Black = 
        fwcyan.plugin.JSprinCore.ReversiDisk.create(ob.color.Black);
    var turnToggle_White = 
        fwcyan.plugin.JSprinCore.ReversiDisk.create(ob.color.White);

    var toggle_put = fwcyan.ButtonToggle.create("Put color", [
        putToggle_Black, putToggle_White, putToggle_Empty, putToggle_None,
    ]);
    toggle_put.change = function(){
        putColor = this._item._value;
    };
    var toggle_turn = fwcyan.ButtonToggle.create("Turn", [
        turnToggle_Black, turnToggle_White,
    ]);
    toggle_turn.change = function(){
        turnColor = this._item._value;
    };

    var apply_edit = fwcyan.Button.create("Apply", 2);
    apply_edit.select = function(){
        var gamestring = ob.getBoard();
        gamestring = gamestring.concat("t");
        if( turnColor === ob.color.Black ){
            gamestring = gamestring.concat("X");
        } else if( turnColor === ob.color.White ){
            gamestring = gamestring.concat("O");
        }
        if(eval_worker !== null){
            eval_worker.terminate();
            eval_worker = null;
            eval_loaded = false;
        }
        og.init(gamestring);
        ceval.setString("Evaluate");
        if( ceval.getScore() !== 0 ){ ceval.select(); }
        popup_edit.close();
    };

    var back_edit = fwcyan.Button.create("Back", -1);
    back_edit.select = function(){ popup_edit.close(); };

    var popup_edit = fwcyan.Popup.create(fwcyan.Template1.create([
        ob,
        fwcyan.Button.create("", 0),
        fwcyan.Button.create("", 0),
        fwcyan.Button.create("", 0),
        fwcyan.Button.create("", 0),
        toggle_put,
        toggle_turn,
        apply_edit,
        back_edit,
    ]));

    fwcyan.setPopups([ popup_edit, ]);

    og.change();
    ceval.select();
};
kurumicl.onload = function(canvas){
    FEcipia.fwcyan = FWcyan(canvas, [JSprinCore]);
    FEcipia.setup(FEcipia.fwcyan);
};
kurumicl.onresize = function(){
    FEcipia.fwcyan.resize();
    FEcipia.fwcyan.draw();
};
