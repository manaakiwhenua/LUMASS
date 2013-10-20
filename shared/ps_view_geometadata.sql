create or replace view geometadata as
select coll_name, oid, covername, base_type, 
       array_to_string(lower_left, ',') as lower_left,
       array_to_string(upper_right, ',') as upper_right, 
       axis_offsets
       from 
          -- coverage name plus bounding box
            ( select id, name as covername
               from ps9_coverage
            )
          as v0 
          inner join 
            ( select coverage_id, lower_left, upper_right
               from ps9_bounding_box
            )
          as v1 on v0.id = v1.coverage_id
	  -- flattened (aggregated) axis offset vectors per coverage  
          inner join 
             (select coverage_id, string_agg(array_to_string(offset_vector, ','), ' ') as axis_offsets
		from  (select id as coverage_id from
			 ps9_coverage
		      )
		   as v1
		   inner join
		      (select id as axis_id, gridded_coverage_id 
			from ps9_grid_axis 
		      )
		   as v2 on v1.coverage_id = v2.gridded_coverage_id 
		   inner join 
		      (select grid_axis_id, offset_vector 
			 from ps9_rectilinear_axis 
		       )
		   as v3 on v2.axis_id = v3.grid_axis_id
		   group by coverage_id
	       )
          as v4 on v1.coverage_id = v4.coverage_id
	  -- rasdaman info, i.e. collection name and OID
          inner join
	       (select tmp_covid, oid, name as coll_name, base_type
	          from (select id as tmp_covid
		         from ps9_coverage
		       )
		     as s1
		     inner join
		       (select storage_id, coverage_id
		         from ps9_range_set
		       )
		     as s2 on s1.tmp_covid = s2.coverage_id
		     inner join
		       (select id, name, oid, base_type
		         from ps9_rasdaman_collection
		       )
		     as s3 on s2.storage_id = s3.id
		  )
	   as v5 on v4.coverage_id = v5.tmp_covid;

          

               
          
             