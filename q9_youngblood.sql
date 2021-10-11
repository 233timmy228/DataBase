SELECT RegionDescription,FirstName,LastName,BirthDate
FROM(SELECT * FROM Employee
              LEFT OUTER JOIN EmployeeTerritory 
              ON Employee.Id=EmployeeTerritory.employeeId
              LEFT OUTER JOIN Territory 
              ON EmployeeTerritory.TerritoryId=Territory.Id
              LEFT OUTER JOIN Region 
              ON Territory.RegionId=Region.Id
    ORDER BY Employee.BirthDate DESC)
GROUP BY RegionId
ORDER BY RegionId;