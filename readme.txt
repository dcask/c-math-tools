test-main
	noisetest
	gain
	a4x
	impulse
	traces
----------------------
noisetest
	setgen
	runreg
	getdata
	noholes
	noisel
результаты в noise-1,noise-2,noise-3,noise-4
----------------------
gain
	setgen
	synchron
	runreg
	getdata
	noholes
	average
	approx
	notline
результаты g-1,g-2,g-3,g-4, /Gains/gain-[1..128]-[1..4]
----------------------
a4x
	setgen
	synchron
	runreg
	getdata
	dft
	a4xan
результаты afcan-[1..4], /AFC/afc-[1..128]-[4000..250]-[1..4]
----------------------
impulse
	setgen
	synchron
	runreg
	getdata
	offsfind
результат impulse-[1..4], imp-[4000..250]-[1..128]-[1..4]
----------------------
traces
	setgen
	synchron
	runreg
	getdata
	offsfind
	deviat
результат traces-[1..4]
