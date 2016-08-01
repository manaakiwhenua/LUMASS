--drop function create_metatable();
create or replace function create_metatable()
returns void as 
$BODY$
	declare
                qhead text;
		qbody text;
		qtmp text;
		t text;
		types text[];
		values text[];
		cols text;
		colvals text;
		rec record;
		metarec record;
		ve integer;
		i integer := 1;
		numtypes integer := 0;
	begin

                -- get a string array of types (attribute names)
                -- and count the attributes
                select array_agg(type) into types 
                  from ps_extra_metadata_type;
                numtypes := array_length(types, 1);
                        
                -- determine whether the flatmetadata table exists
                select count(relname) into ve 
                  from pg_catalog.pg_class
                    where relname = 'tmp_flatmetadata';

                -- delete table, if it exists
                -- note, we cannot use a view, since they
                -- 're not editable (at least in pg)
                if ve > 0 then
                   drop table tmp_flatmetadata;
                end if;

                -- create the table ------------------------------------------
                -- prepare the sql statement
                qhead := 'create table tmp_flatmetadata ( ' ||
                         'oid integer NOT NULL, ';

                foreach t in array types loop
                        qhead := qhead || format('%I', t) || ' text, ';

                        if i = numtypes then
                                qhead := qhead || 
                                'constraint tmp_flatmetadata_pkey primary key (oid))';
                        end if;
                                 
                        i := i + 1;
                end loop;
                -- actually create the table
                execute qhead;

                -- fill the table ----------------------------------------
	
                -- iterate over coverages (ids), fetch metadata and 
                -- put it into the prepared flat metadata table
                qtmp := 'select oid from ps_rasdaman_collection as s1 order by s1.oid';
                for rec in execute qtmp loop
		    -- fetch metadata for an individual coverage
                    select oid, 
                           array_agg(type) as attributes,
                           array_agg(value) as attrvals 
                      from extrametadata 
                     where oid = rec.oid group by oid into metarec; 


		    -- prepare the columns and values part of the 
		    -- insert query	
	            cols := '(oid,';
	            colvals := ' values (' || metarec.oid || ',';
		    numtypes := array_length(metarec.attributes, 1);

		    if numtypes is null or numtypes < 1 then
			continue;
		    end if;

		    -- integrate actual columns and values with query string
	            for elem in 1..numtypes loop
			cols := cols || format('%I', metarec.attributes[elem]);
			colvals := colvals || 
				format('%L', metarec.attrvals[elem]);

			-- don't forget to close the set of values once 
			-- the number of max elems is reached
			if elem < numtypes then
				cols := cols || ', ';
				colvals := colvals || ', ';
			else
				colvals := colvals || ')';
				cols := cols || ')';
			end if;
	            end loop;

		    -- run the insert query
                    qbody := 'insert into tmp_flatmetadata ' || 
			      cols || colvals;
		    execute qbody;
                end loop;
                return;
	end;
$BODY$ 
language plpgsql;

-- select create_metatable();
-- select * from geometadata as t1
--        left join tmp_flatmetadata as t2
--        on t1.oid = t2.oid;
