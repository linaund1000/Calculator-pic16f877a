**to compile in terminal ->
    xc8-cc -mcpu=16f877a -O1 -o calculator789.hex main789.c


**result ->
  main.c:180:: warning: (765) degenerate unsigned comparison
  main.c:190:: warning: (765) degenerate unsigned comparison
  main.c:236:: warning: (765) degenerate unsigned comparison
  main.c:251:: warning: (765) degenerate unsigned comparison
  
  16F877A Memory Summary:
      Program space        used   BF6h (  3062) of  2000h words   ( 37.4%)
      Data space           used    69h (   105) of   170h bytes   ( 28.5%)
      EEPROM space         used     0h (     0) of   100h bytes   (  0.0%)
      Configuration bits   used     1h (     1) of     1h word    (100.0%)
      ID Location space    used     0h (     0) of     4h bytes   (  0.0%)
