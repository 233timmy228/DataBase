SELECT Id,OrderDate,
            (LAG(OrderDate,1,orderDate) OVER(ORDER BY OrderDate)),
             printf("%.2f",julianday(OrderDate)-julianday(LAG(OrderDate,1) OVER(ORDER BY OrderDate))) 
FROM 'Order' 
WHERE CustomerId='BLONP'
ORDER BY OrderDate 
LIMIT 10;