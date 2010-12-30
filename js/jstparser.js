/*
Copyright 2008, mark turansky (www.markturansky.com)
Copyright 2010, Andrew Kelley (superjoesoftware.com)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

The Software shall be used for Good, not Evil.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE. 

(This is the license from www.json.org and I think it's awesome)
*/

String.prototype.endsWith = function endsWith(c) {
    if (this.charAt(this.length - 1) == c) {
        return true;
    } else {
        return false;
    }
};

String.prototype.startsWith = function startsWith(c) {
    if (this.charAt(0) == c) {
        return true;
    } else {
        return false;
    }
};

RegExp.quote = function(str) {
    return str.replace(/([.?*+\^$\[\]\\(){}\-])/g, "\\$1");
};

String.prototype.replaceAll = function replaceAll(a, b) {
    return this.replace(new RegExp(RegExp.quote(a), 'g'), b);
};

var Jst = function () {
    // private variables:
    var that; // reference to the public object
    
    // all write and writeln functions eval'd by Jst
    // concatenate to this internal variable
    var html = "";

    // private functions
    function CharacterStack(str) {
        var i;

        this.characters = [];
        this.peek = function peek() {
            return this.characters[this.characters.length - 1];
        };
        this.pop = function pop() {
            return this.characters.pop();
        };
        this.push = function push(c) {
            this.characters.push(c);
        };
        this.hasMore =  function hasMore() {
            if (this.characters.length > 0) {
                return true;
            } else {
                return false;
            }
        };

        for (i = str.length; i >= 0; i -= 1) {
            this.characters.push(str.charAt(i));
        }
    }

    function StringWriter() {
        this.str = "";
        this.write = function write(s) {
            this.str += s;
        };
        this.toString = function toString() {
            return this.str;
        };
    }

    function parseScriptlet(stack) {
        var fragment = new StringWriter();
        var c; // character

        while (stack.hasMore()) {
            if (stack.peek() == '%') { //possible end delimiter
                c = stack.pop();
                if (stack.peek() == '>') { //end delimiter
                    // pop > so that it is not available to main parse loop
                    stack.pop();
                    if (stack.peek() == '\n') {
                        fragment.write(stack.pop());
                    }
                    break;
                } else {
                    fragment.write(c);
                }
            } else {
                fragment.write(stack.pop());
            }
        }
        return fragment.toString();
    }

    function isOpeningDelimiter(c) {
        if (c == "<" || c == "%lt;") {
            return true;
        } else {
            return false;
        }
    }

    function isClosingDelimiter(c) {
        if (c == ">" || c == "%gt;") {
            return true;
        } else {
            return false;
        }
    }

    function appendExpressionFragment(writer, fragment, jstWriter) {
        var i,j;
        var c;

        // check to be sure quotes are on both ends of a string literal
        if (fragment.startsWith("\"") && !fragment.endsWith("\"")) {
            //some scriptlets end with \n, especially if the script ends the file
            if (fragment.endsWith("\n") && fragment.charAt(fragment.length - 2) == '"') {
                //we're ok...
            } else {
                throw { "message":"'" + fragment + "' is not properly quoted"};
            }
        }

        if (!fragment.startsWith("\"") && fragment.endsWith("\"")) {
            throw { "message":"'" + fragment + "' is not properly quoted"};
        }

        // print or println?
        if (fragment.endsWith("\n")) {
            writer.write(jstWriter + "ln(");
            //strip the newline
            fragment = fragment.substring(0, fragment.length - 1);
        } else {
            writer.write(jstWriter + "(");
        }

        if (fragment.startsWith("\"") && fragment.endsWith("\"")) {
            //strip the quotes
            fragment = fragment.substring(1, fragment.length - 1);
            writer.write("\"");
            for (i = 0; i < fragment.length; i += 1) {
                c = fragment.charAt(i);
                if (c == '"') {
                    writer.write("\\");
                    writer.write(c);
                }
            }
            writer.write("\"");
        } else {
            for (j = 0; j < fragment.length; j += 1) {
                writer.write(fragment.charAt(j));
            }
        }

        writer.write(");");
    }

    function appendTextFragment(writer, fragment) {
        var i;
        var c;

        if (fragment.endsWith("\n")) {
            writer.write("writeln(\"");
        } else {
            writer.write("write(\"");
        }

        for (i = 0; i < fragment.length; i += 1) {
            c = fragment.charAt(i);
            if (c == '"') {
                writer.write("\\");
            }
            // we took care of the line break with print vs. println
            if (c != '\n' && c != '\r') {
                writer.write(c);
            }
        }

        writer.write("\");");
    }

    function parseExpression(stack) {
        var fragment = new StringWriter();
        var c;

        while (stack.hasMore()) {
            if (stack.peek() == '%') { //possible end delimiter
                c = stack.pop();
                if (isClosingDelimiter(stack.peek())) { //end delimiter
                    //pop > so that it is not available to main parse loop
                    stack.pop();
                    if (stack.peek() == '\n') {
                        fragment.write(stack.pop());
                    }
                    break;
                } else {
                    fragment.write("%");
                }
            } else {
                fragment.write(stack.pop());
            }
        }

        return fragment.toString();
    }

    function parseText(stack) {
        var fragment = new StringWriter();
        var c,d;

        while (stack.hasMore()) {
            if (isOpeningDelimiter(stack.peek())) { //possible delimiter
                c = stack.pop();
                if (stack.peek() == '%') { // delimiter!
                    // push c onto the stack to be used in main parse loop
                    stack.push(c);
                    break;
                } else {
                    fragment.write(c);
                }
            } else {
                d = stack.pop();
                fragment.write(d);
                if (d == '\n') { //done with this fragment.  println it.
                    break;
                }
            }
        }
        return fragment.toString();
    }

    function safeWrite(s) {
        s = s.toString();
        s = s.replaceAll('&', '&amp;');
        s = s.replaceAll('"', '&quot;');
        s = s.replaceAll('<', '&lt;');
        s = s.replaceAll('>', '&gt;');
        html += s;
    }

    function safeWriteln(s) {
        safeWrite(s + "\n");
    }

    function write(s) {
        html += s;
    }

    function writeln(s) {
        write(s + "\n");
    }

    that = {
        // public methods:
        // pre-compile a template for quicker rendering. save the return value and 
        // pass it to evaluate.
        compile: function (src) {
            var stack = new CharacterStack(src);
            var writer = new StringWriter();

            var c;
            var fragment;
            while (stack.hasMore()) {
                if (isOpeningDelimiter(stack.peek())) { //possible delimiter
                    c = stack.pop();
                    if (stack.peek() == '%') { //delimiter!
                        c = stack.pop();
                        if (stack.peek() == "=") {
                            // expression, escape all html
                            stack.pop();
                            fragment = parseExpression(stack);
                            appendExpressionFragment(writer, fragment,
                                "safeWrite");
                        } else if (stack.peek() == "+") {
                            // expression, don't escape html
                            stack.pop();
                            fragment = parseExpression(stack);
                            appendExpressionFragment(writer, fragment,
                                "write");
                        } else {
                            fragment = parseScriptlet(stack);
                            writer.write(fragment);
                        }
                    } else {  //not a delimiter
                        stack.push(c);
                        fragment = parseText(stack);
                        appendTextFragment(writer, fragment);
                    }
                } else {
                    fragment = parseText(stack);
                    appendTextFragment(writer, fragment);
                }
            }
            return writer.toString();
        },

        // evaluate a pre-compiled script. recommended approach
        evaluate: function (script, args) {
            with(args) {
                html = "";
                eval(script);
                return html;
            }
        },

        // if you're lazy, you can use this
        evaluateSingleShot: function (src, args) {
            return this.evaluate(this.compile(src), args);
        }
    };
    return that;
}();
