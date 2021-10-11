SELECT DISTINCT shipName ,SUBSTR(shipName,1,INSTR(shipName,'-')-1) 
FROM 'Order' 
WHERE shipName LIKE "%-%" 
ORDER BY shipName;