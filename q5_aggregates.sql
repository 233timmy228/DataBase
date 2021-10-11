SELECT CategoryName,COUNT(CategoryName),ROUND(AVG(UnitPrice),2),MIN(UnitPrice),MAX(UnitPrice),SUM(UnitsOnOrder)
FROM Product LEFT OUTER JOIN Category 
WHERE CategoryId=Category.Id 
GROUP BY Category.Id 
HAVING COUNT(CategoryName)>10 
ORDER BY CategoryId;