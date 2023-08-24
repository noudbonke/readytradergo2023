for filename in autotrader arbtr_dyn_FAK arbtr_dyn_GFD arbtr_fix_FAK arbtr_fix_GFD hybrid basic_plus basic_plus_polyorder autotrader 
do 
	mv build/$filename .
done

python rtg.py run arbtr_dyn_GFD hybrid basic_plus basic_plus_polyorder autotrader
