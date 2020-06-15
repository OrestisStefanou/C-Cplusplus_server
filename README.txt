Orestis Stefanou
AM:1115201700217

Me tin entoli make dimiurgountai ta ektelesima arxia.Prota trexume to script gia tin dimiurgia ton directories.Paradigma ektelesis script:
./create_infiles.sh diseases.txt countries.txt ./Countries 10 10
Paradigma ektelesis efarmogis:
1)Trexume ton whoServer me tin entoli:./WhoServer -q 27123 -s 27124 -w 4 -b 10
2)Trexume ton master me tin entoli:./master -w 5 -b 100 -s 127.0.0.1 -p 27124 -i ./Countries
3)Trexume ton whoClient me tin entoli:./whoClient -q Queries.txt -w 4 -sp 27123 -sip 127.0.0.1

Me tin entoli make clean diagrafontai ta eketelesima arxia kai o fakelos me ta logfiles

Domes pu ilopoiisa:
Oi domes tou master kai ton worker ine oi idies me tis 2is ergasias
Domes WhoServer:
1)Mia apla sindedemeni lista stin opoia kratai tin ip address kai to port sto opoio akoune oi workers
2)Ena hashtable sto opoio kratame ta statistika gia kathe xora kai tis plirofories gia ton worker pou ine ipeuthinos gia tin xora auti.

Genikes Domes:
1)I domi queryinfo periexei oles tis plirofories pu xriazete o worker gia na apantisi se kapio erotima.O whoServer stelnei meso tu diktiou minima pu periexei tis plirofories tis domis queryinfo stous workers.O whoServer dimiurgi to minima me ti xrisi tis sinartisis fprintf kai o worker diavazi tis metavlites me tin xrisi tis sinartisis fscanf.


Sigxronismos ton thread tou WhoServer:
Gia ton sigxronismo ton thread xrisimopoio 2 metavlites sinthikis(cvar1,cvar2) kai ena mutex(mtx).Yparxi enas koinos buffer anamesa sta noimata opou iparxoun ta file descriptors tis neas sindesis.To main thread prota elegxi an o buffer ine gematos kai ine kani wait stin metavliti sinthikis cvar2.An den ine vazi to file descriptor ston buffer kai kani signal stin metavliti sinthikis cvar1 gia na analavi kapio nima tin sindesi.Ola ta nimata(ektos tu main) kanun wait stin metavliti sinithkis cvar1.


Leitourgies:
1)O master stelnei ta onomata me tis xores pu ine ipeuthinos o kathe worker meso named pipes.Oi workers me tin sira tus diavazun ta directories gemizun tis domes tous kai stelnoun ta statistika tou kathe arxiou mazi me to port sto opoio kanun listen ston WhoServer,o opoios ta apothikeui stis dikes tu domes.

2)Erotimata:
/diseaseFrequency:An den dothi to onoma xoras o WhoServer stelnei se olus tus workers aitima me tin xrisi tis domis query info.Oi workers meso ton domon tous apantoun sto erotima kai o server proothi tin apantisi ston whoClient.An dothi xora tote o whoServer stelnei aitima mono ston worker pou diaxirizete tin xora auti.

/topk-AgeRanges:O whoServer me ta statistika pu tu estilan oi workers mpori na apantisi sto erotima apeuthias ston whoClient.


/searchPatientRecord:O whoServer stelnei aitima stus workers(ena worker kathe fora).An kapoios worker apantisi thetika(vrike to PatientRecord pu zitisame),tote den stelnume aitima stus ypoloipous workers kai proothoume tin apantisi ston whoClient.An kanenas worker den apantisi thetika enimeronoume ton whoClient oti i eggrafi den vrethike.


/numPatientAdmissions:O whoServer me ta statistika pu tu estilan oi workers mpori na apantisi apeuthias ston whoClient.


/numPatientDischarges:n den dothi to onoma xoras o WhoServer stelnei se olus tus workers aitima me tin xrisi tis domis query info.Oi workers meso ton domon tous apantoun sto erotima kai o server proothi tin apantisi ston whoClient.An dothi xora tote o whoServer stelnei aitima mono ston worker pou diaxirizete tin xora auti.


WhoServer:
O whoServer dimiurgei 2 listening sockets.Ena gia ta statistika kai ena gia ta queries.Me tin xrisi tis sinartisis poll parakoluthi auta ta 2 sockets.Otan kani accept kapoia sindesi topotheti ston buffer to neo file descriptor pu epestrepse i accept kai kapoio nima eksipireti tin sindesi.

Worker:
O worker episis me tin xrisi tis sinartisis poll parakoluthi oxi mono to socket sto opoio kani listen alla kai ta file descriptors ton sindeseon pu dexete(ta file descriptors pou epistrefi i accept).

O logos pu epeleksa tin xrisi tis sinartisis poll ine epidi ine sxetika apli kai sxetika apodotiki otan o arithmos ton file descriptor pu parakolouthoume den ine poli megalos.Sto arxio network.h iparxoun oi sinartisis pu exun na kanoun me tin poll.
