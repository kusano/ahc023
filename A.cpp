#include <iostream>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <functional>
#include <tuple>
using namespace std;
using namespace std::chrono;

const double TIME = 1.8;

const int expN = 1024;
const double expX = 16;
int expT[expN];

void my_exp_init()
{
    for (int x=0; x<expN; x++)
    {
        double x2 = (double)-x/expN*expX;
        double e = exp(x2)*0x80000000+.5;
        if (e>=0x7fffffff)
            expT[x] = 0x7fffffff;
        else
            expT[x] = int(e);
    }
}

//  exp(t)*0x80000000;
int my_exp(double x)
{
    int x2 = int(x/-expX*expN+.5);
    if (x2<0)
        return expT[0];
    if (x2>=expN)
        return expT[expN-1];
    return expT[x2];
}

int xor64() {
    static uint64_t x = 88172645463345263ULL;
    x ^= x<<13;
    x ^= x>> 7;
    x ^= x<<17;
    return int(x&0x7fffffff);
}

struct Node
{
    int x, y;
    int cn;
    int ln;
    vector<Node *> C;
};

int main()
{
    // 入力
    int T, H, W, y0;
    cin>>T>>H>>W>>y0;
    vector<vector<int>> h(H-1, vector<int>(W));
    for (int y=0; y<H-1; y++)
    {
        string t;
        cin>>t;
        for (int x=0; x<W; x++)
            h[y][x] = t[x]-'0';
    }
    vector<vector<int>> v(H, vector<int>(W-1));
    for (int y=0; y<H; y++)
    {
        string t;
        cin>>t;
        for (int x=0; x<W-1; x++)
            v[y][x] = t[x]-'0';
    }
    int K;
    cin>>K;
    vector<int> S(K), D(K);
    for (int k=0; k<K; k++)
    {
        cin>>S[k]>>D[k];
        S[k]--;
        D[k]--;
    }

    system_clock::time_point start = system_clock::now();

    my_exp_init();

    const int DX[] = {1, -1, 0, 0};
    const int DY[] = {0, 0, 1, -1};

    // 木の作成
    Node *tree;
    {
        tree = new Node();
        tree->x = 0;
        tree->y = y0;

        vector<Node *> V;
        V.push_back(tree);
        vector<vector<bool>> U(H, vector<bool>(W));
        U[y0][0] = true;

        for (int i=0; i<(int)V.size(); i++)
        {
            Node *n = V[i];
            int r = xor64()%4;

            for (int d=0; d<4; d++)
            {
                int tx = n->x+DX[d];
                int ty = n->y+DY[d];
                if (0<=tx && tx<W &&
                    0<=ty && ty<H &&
                    !U[ty][tx] &&
                    (n->y==ty || h[min(n->y, ty)][n->x]==0) &&
                    (n->x==tx || v[n->y][min(n->x, tx)]==0))
                {
                    U[ty][tx] = true;

                    Node *c = new Node();
                    c->x = tx;
                    c->y = ty;

                    n->C.push_back(c);
                    V.push_back(c);
                }
            }
        }

        function<void(Node *)> f = [&](Node *n)
        {
            n->cn = 1;
            n->ln = n->C.empty()?1:0;

            for (Node *c: n->C)
            {
                f(c);
                n->cn += c->cn;
                n->ln += c->ln;
            }
        };
        f(tree);
    }

    /*
    // 木の表示
    {
        vector<string> B(H*2+1, string(W*4+1, ' '));
        B[0][0] = B[0][W*4] = B[H*2][0] = B[H*2][W*4] = '+';
        for (int x=1; x<W*4; x++)
            B[0][x] = B[H*2][x] = '-';
        for (int y=1; y<H*2; y++)
        {
            if (y!=y0*2+1)
                B[y][0] = '|';
            B[y][W*4] = '|';
        }

        function<void(Node *)> f = [&](Node *n)
        {
            B[n->y*2+1][n->x*4+2] = '+';
            for (Node *c: n->C)
            {
                if (n->x==c->x)
                    B[(n->y+c->y)+1][n->x*4+2] = '|';
                else
                    for (int i=0; i<3; i++)
                        B[n->y*2+1][(n->x+c->x)*2+i+1] = '-';
                f(c);
            }
        };
        f(tree);

        for (string b: B)
            cout<<b<<endl;
    }
    */

    // ここを根とする部分木に植える
    vector<Node *> ST;
    {
        function<void(Node *)> f = [&](Node *n)
        {
            if (n->C.size()==0)
                return;
            if (n->C.size()==1)
                f(n->C[0]);
            if (n->C.size()>=2)
            {
                for (Node *c: n->C)
                {
                    if (c->cn<32 || n->ln==1)
                        ST.push_back(c);
                    else
                        f(c);
                }
            }
        };
        f(tree);
    }
    int sn = (int)ST.size();

    // 区間の区切り
    vector<vector<int>> SP(sn);
    for (int i=0; i<sn; i++)
    {
        SP[i].push_back(0);
        SP[i].push_back(T);
        for (int j=0; j<7; j++)
            SP[i].push_back(xor64()%(T-1)+1);
        sort(SP[i].begin(), SP[i].end());
    }

    // 長さの降順にソート
    vector<int> I;
    for (int i=0; i<K; i++)
        I.push_back(i);
    sort(I.begin(), I.end(), [&](int x, int y){return D[x]-S[x]>D[y]-S[y];});

    // 長さの降順に、なるべく短い区間に割り当てる
    vector<vector<vector<int>>> C(sn);
    for (int i=0; i<sn; i++)
        C[i].resize(SP[i].size());
    for (int i: I)
    {
        int bl = T+1;
        int bs = 0;
        int bp = 0;
        for (int s=0; s<sn; s++)
            for (int p=0; p<(int)SP[s].size()-1; p++)
                if ((int)C[s][p].size()<ST[s]->cn &&
                    SP[s][p]<=S[i] && D[i]<SP[s][p+1])
                {
                    int l = SP[s][p+1]-SP[s][p];
                    if (l<bl)
                    {
                        bl = l;
                        bs = s;
                        bp = p;
                    }
                }
        if (bl<T+1)
            C[bs][bp].push_back(i);
    }

    // 配置
    vector<int> ans_k, ans_x, ans_y, ans_s;

    for (int s=0; s<sn; s++)
    {
        vector<int> X, Y, DD;
        function<void(Node *, int)> f = [&](Node *n, int d)
        {
            X.push_back(n->x);
            Y.push_back(n->y);
            DD.push_back(d);
            for (Node *c: n->C)
                f(c, d+1);
        };
        f(ST[s], 0);

        vector<tuple<int, int, int>> DXY;
        for (int i=0; i<(int)X.size(); i++)
            DXY.push_back({DD[i], X[i], Y[i]});
        sort(DXY.begin(), DXY.end());
        for (int i=0; i<(int)X.size(); i++)
        {
            DD[i] = get<0>(DXY[i]);
            X[i] = get<1>(DXY[i]);
            Y[i] = get<2>(DXY[i]);
        }

        for (int p=0; p<(int)SP[s].size()-1; p++)
        {
            sort(C[s][p].begin(), C[s][p].end(), [&](int x, int y){return D[x]<D[y];});
            for (int i=0; i<(int)C[s][p].size(); i++)
            {
                ans_k.push_back(C[s][p][i]);
                ans_x.push_back(X[i]);
                ans_y.push_back(Y[i]);
                ans_s.push_back(SP[s][p]);
            }
        }
    }

    // 出力
    cout<<ans_k.size()<<endl;
    for (int i=0; i<(int)ans_k.size(); i++)
        cout<<ans_k[i]+1<<" "<<ans_y[i]<<" "<<ans_x[i]<<" "<<ans_s[i]+1<<endl;

    system_clock::time_point end = system_clock::now();

    // 情報出力
    {
        // d
        int d = 0;
        vector<vector<bool>> M(H+1, vector<bool>(W+1));
        for (int x=0; x<W+1; x++)
            M[0][x] = M[H][x] = true;
        for (int y=0; y<H+1; y++)
            M[y][0] = M[y][W] = true;
        for (int y=0; y<H-1; y++)
            for (int x=0; x<W; x++)
                if (h[y][x]!=0)
                    M[y+1][x] = M[y+1][x+1] = true;
        for (int y=0; y<H; y++)
            for (int x=0; x<W-1; x++)
                if (v[y][x]!=0)
                    M[y][x+1] = M[y+1][x+1] = true;
        for (int y=1; y<H; y++)
            for (int x=1; x<W; x++)
                if (!M[y][x])
                {
                    int t = 9999;
                    for (int dy=-4; dy<=4; dy++)
                        for (int dx=-4; dx<=4; dx++)
                        {
                            int tx = x+dx;
                            int ty = y+dy;
                            if (0<=tx && tx<=W &&
                                0<=ty && ty<=H &&
                                M[ty][tx])
                                t = min(t, abs(dx)+abs(dy));
                        }
                    d = max(d, t);
                }

        // L
        int L = 0;
        for (int k=0; k<K; k++)
            L += D[k]-S[k]+1;

        // 実行時間
        double time = chrono::duration_cast<chrono::nanoseconds>(end-start).count()*1e-9;

        // スコア
        int score = 0;
        for(int k: ans_k)
            score += 1000000/(H*W*T)*(D[k]-S[k]+1);

        fprintf(stderr, "%d %4d %5.3f %5.3f %6d\n", d, K, (double)L/(H*W*T), time, score);
    }
}
