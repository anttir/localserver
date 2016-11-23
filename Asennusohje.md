
Avaa PowerShell, mene hakemistoon:
```
C:\+Ohjelmointi\GitHub\localserver>
```

Asenna Node-kamat (jos ei ole vielä asennettu):

```
npm install particle-api-js yargs node-persist
```

Tallenna Particlen kehitysympäristön salasana:

```
node .\liveimu.js --login antti.rasi@gmail.com oXv3rr5FJsRm
```
jatkossa 
```
node .\liveimu.js
```
Mene selaimella osoitteeseen: http://localhost:8070/index.html

--

Debug:
- Ledi vilkkuu todella hitaasti: vastarilta ei tule dataa (lukitsee koko loopin ~sekunnin tauolle)
- Ledi vilkkuu nopeasti: kaikki pelittää
- Ledi ei vilku (voi jäädä päälle tai pois): palvelin ei vastaa