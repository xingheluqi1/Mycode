def dfs(x):
	if x==n+1:
		global ans
		ans+=1
	for y in range(1,n+1):
		if vis1[y] or vis2[x+y] or vis3[x-y+n]:
			continue
		vis1[y]=vis2[x+y]=vis3[x-y+n]=True
		dfs(x+1)
		vis1[y]=vis2[x+y]=vis3[x-y+n]=False
ans=0
n=int(input())
vis1=[False]*(n+1)
vis2=[False]*(2*n+1)
vis3=[False]*(2*n+1)
dfs(1)
print(ans)