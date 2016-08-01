create or replace view extrametadata as
select oid, type, value   
  -- create a table of extra metadata plus type per coverage id
  from (select coverage_id, type, value
         from (select coverage_id, metadata_type_id, value
                 from ps_extra_metadata
              )
           as v1 
           join 
              (select id, type
                from ps_extra_metadata_type
              )
           as v2 on v1.metadata_type_id = v2.id
       )
    as v3
    -- now join the rasdaman OID to the coverage id
    natural join 
       (select coverage_id, storage_id 
         from ps_range_set 
       )
    as v4  
    inner join
       (select id, oid
          from ps_rasdaman_collection
       )
    as v5 on v4.storage_id = v5.id;
        