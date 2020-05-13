let code = `e	0110
t	0010
a	11111
r	11001
i	11000
o	10110
space	10100
n	10011
s	10010
h	10000
E	01011
UNICODE16	01000
T	00110
d	00001
A	00000
l	111100
R	110100
I	101111
O	101011
N	101010
S	100011
H	100010
u	010101
w	010011
m	001111
f	000111
c	000101
D	000100
g	1111010
L	1110111
_	1110110
-	1101111
y	1101011
p	1011101
:	0111110
,	0111101
)	0111100
/	0111011
.	0111010
(	0111001
b	0111000
U	0101001
k	0101000
W	0100101
M	0011101
F	0001101
C	0001100
G	11110110
?	11101011
>	11101010
[	11101001
@	11101000
;	11100111
+	11100110
=	11100101
<	11100100
&	11100011
%	11100010
*	11100001
'	11100000
$	11011101
quote	11011100
UNICODE24	11011011
]	11011010
!	11011001
Y	11011000
v	11010100
P	10111001
B	01111110
K	01001001
\\	111101110
#	110101011
V	110101010
}	1111011111
|	1111011110
~	010010000
^	001110011
{	001110001
\`	001110000
3	1011100010
0	1011100001
2	0111111111
1	0111111110
j	0100100011
x	10111000111
5	0011100101
4	0011100100
7	10111000001
6	10111000000
9	01111111011
8	01111111010
q	01111111001
z	01111111000
J	01001000101
X	01001000100
Q	101110001100
Z	1011100011011
LF	101110001101011
HT	101110001101010
CR	10111000110100`;

code = code
	.split(/\r?\n/)
	.map(x => {
		let [a, b] = x.trim().split(/\s+/);
		return { id: a, bin: b };
	});

let codeMap = {};
code.map(x => {
	codeMap[x.bin] = x.id;
});


let tabsToDo = { '': true };
let tabs = {};

function decode(code) {

	let repeat;
	let output = [];
	let startingCode = code;

	do {
		repeat = false;
		for (let i = 4; i <= code.length; i++) {
			let part = code.substr(0, i);
			if (part in codeMap) {
				output.push(codeMap[part]);
				code = code.substr(i);
				repeat = true;
				break;
			}
		}
	} while (repeat);

	if (!(code in tabs) && !(code in tabsToDo)) {
		tabsToDo[code] = true;
	}

	return {
		output: output.join(', '),
		next: code
	};
}

function createTab(state) {
	let tab = {};
	for (let i = 0; i < 256; i++) {
		let byte = i.toString(2);
		while (byte.length < 8) byte = '0' + byte;
		let code = state + byte;
		tab[byte] = decode(code);
	}
	return tab;
}

while (Object.keys(tabsToDo).length > 0) {
	tabName = Object.keys(tabsToDo)[0];
	tabs[tabName] = createTab(tabName);
	delete tabsToDo[tabName];
}

//console.log(tabs);
//console.log(Object.keys(tabs).length);

let invTabs = {};
tabsToDo = { '': true };

function createInvTab(state) {
	let tab = {};
	for (let i in code) {
		let fullCode = state + code[i].bin;
		let output = [];
		while (fullCode.length >= 8) {
			output.push(fullCode.substr(0, 8));
			fullCode = fullCode.substr(8);
		}
		tab[code[i].id] = {
			output: output.join(', '),
			next: fullCode
		};
		if (!(fullCode in invTabs) && !(fullCode in tabsToDo)) {
			tabsToDo[fullCode] = true;
		}
	}
	return tab;
}

while (Object.keys(tabsToDo).length > 0) {
	tabName = Object.keys(tabsToDo)[0];
	invTabs[tabName] = createInvTab(tabName);
	delete tabsToDo[tabName];
}

console.log(invTabs);
console.log(Object.keys(invTabs).length);
