module Task1_tcp where
task1_tcp::[[String]]->Int
task1_tcp tr = sum $ map  (\wds->(read $ wds !! 10)::Int) $ filter (\wds->and $ map (\(i,w)->wds !! i==w) [(0,"r"),(6,"3"),(16,"1"),(8,"tcp")]) tr
