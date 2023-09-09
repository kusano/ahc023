#include <iostream>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <functional>
#include <tuple>
#include <set>
using namespace std;
using namespace std::chrono;

const double TIME = 1.9;

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
        vector<vector<int>> score(H, vector<int>(W));
        vector<vector<vector<Node *>>> st(H, vector<vector<Node *>>(W));

        function<void(Node *)> f = [&](Node *n)
        {
            for (Node *c: n->C)
            {
                f(c);
                score[n->y][n->x] += score[c->y][c->x];
                st[n->y][n->x].insert(st[n->y][n->x].begin(), st[c->y][c->x].begin(), st[c->y][c->x].end());
            }
            int s = int((98.077-n->cn*0.5668)*25*n->cn);
            if (s>score[n->y][n->x])
            {
                score[n->y][n->x] = s;
                st[n->y][n->x] = vector<Node *>(1, n);
            }
        };
        f(tree);
        ST = st[tree->y][tree->x];
    }
    int sn = (int)ST.size();

    // 長さの降順にソート
    vector<int> I;
    for (int i=0; i<K; i++)
        I.push_back(i);
    sort(I.begin(), I.end(), [&](int x, int y){return D[x]-S[x]>D[y]-S[y];});

    // D別
    vector<vector<int>> I2(T);
    for (int i=0; i<K; i++)
        I2[D[i]].push_back(i);

    // 区間の区切り
    vector<vector<int>> SP(sn);
    for (int i=0; i<sn; i++)
    {
        SP[i].push_back(0);
        SP[i].push_back(T);
        for (int j=0; j<7; j++)
            while (true)
            {
                int t = xor64()%(T-1)+1;
                bool ok = true;
                for (int p: SP[i])
                    if (p==t)
                        ok = false;
                if (ok)
                {
                    SP[i].push_back(t);
                    break;
                }
            }
        sort(SP[i].begin(), SP[i].end());
    }

    vector<bool> used(K);
    int score = 0;

    // 長さの降順に、なるべく短い区間に割り当てる
    vector<vector<vector<int>>> C(sn);
    for (int i=0; i<sn; i++)
        C[i].resize(SP[i].size()-1);

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
        {
            used[i] = true;
            score += 25*(D[i]-S[i]+1);
            C[bs][bp].push_back(i);
        }
    }

    double time;
    double temp_inv;
    int iter;

    int score_best = score;
    vector<vector<int>> SPbest = SP;
    vector<vector<vector<int>>> Cbest = C;

    for (iter=0; ; iter++)
    {
        if (iter%0x100==0)
        {
            system_clock::time_point now = system_clock::now();
            time = chrono::duration_cast<chrono::nanoseconds>(now-start).count()*1e-9/TIME;
            if (time>1.0)
                break;
            double temp = 100.*(1.0-time);
            temp_inv = 1./temp;
        }

        int sr = xor64()%sn;

        int score_old = score;
        vector<int> SP_old = SP[sr];
        vector<vector<int>> C_old = C[sr];
        vector<int> used_hist;

        // 割り当てから区間を削除
        auto remove = [&](int p)
        {
            for (int c: C[sr][p])
            {
                used[c] = false;
                used_hist.push_back(c);
                score -= 25*(D[c]-S[c]+1);
            }
            C[sr].erase(C[sr].begin()+p);
        };

        // 割り当てに区間を追加
        // Pに追加してから呼ぶ
        auto add = [&](int p)
        {
            vector<int> cand;
            for (int t=SP[sr][p]+1; t<SP[sr][p+1]; t++)
                for (int i: I2[t])
                    if (!used[i] && S[i]>=SP[sr][p])
                        cand.push_back(i);
            sort(cand.begin(), cand.end(), [&](int x, int y){return D[x]-S[x]>D[y]-S[y];});

            C[sr].insert(C[sr].begin()+p, vector<int>());
            for (int c=0; c<(int)cand.size() && c<ST[sr]->cn; c++)
            {
                int i = cand[c];

                C[sr][p].push_back(i);

                used[i] = true;
                used_hist.push_back(i);
                score += 25*(D[i]-S[i]+1);
            }
        };

        switch (xor64()%3)
        {
        case 0:
        {
            // 追加
            int pr = xor64()%int(SP[sr].size()-1)+1;
            if (SP[sr][pr]-SP[sr][pr-1]==1)
                continue;
            int np = xor64()%(SP[sr][pr]-SP[sr][pr-1]-1)+SP[sr][pr-1]+1;
            SP[sr].insert(SP[sr].begin()+pr, np);
            remove(pr-1);
            add(pr-1);
            add(pr);
            break;
        }
        case 1:
        {
            // 削除
            if (SP[sr].size()==2)
                continue;
            int pr = xor64()%int(SP[sr].size()-2)+1;
            SP[sr].erase(SP[sr].begin()+pr);
            remove(pr);
            remove(pr-1);
            add(pr-1);
            break;
        }
        case 2:
        {
            // 移動
            if (SP[sr].size()==2)
                continue;
            int pr = xor64()%int(SP[sr].size()-2)+1;
            int np = xor64()%(SP[sr][pr+1]-SP[sr][pr-1]-1)+SP[sr][pr-1]+1;
            SP[sr][pr] = np;
            remove(pr);
            remove(pr-1);
            add(pr-1);
            add(pr);
            break;
        }
        }

        if (score>score_old ||
            my_exp((score-score_old)*temp_inv)>xor64())
        {
            if (score>score_best)
            {
                score_best = score;
                SPbest = SP;
                Cbest = C;

                // 後半は全体を更新し直す
                if (time>0.5)
                {
                    for (int s=0; s<(int)C.size(); s++)
                        for (int p=0; p<(int)C[s].size(); p++)
                            C[s][p].clear();
                    score = 0;
                    for (int i=0; i<K; i++)
                        used[i] = false;
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
                        {
                            used[i] = true;
                            score += 25*(D[i]-S[i]+1);
                            C[bs][bp].push_back(i);
                        }
                    }

                    if (score>score_best)
                    {
                        score_best = score;
                        SPbest = SP;
                        Cbest = C;
                    }
                }
            }
        }
        else
        {
            score = score_old;
            SP[sr] = SP_old;
            C[sr] = C_old;
            for (int c: used_hist)
                used[c] = !used[c];
        }
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

        for (int p=0; p<(int)SPbest[s].size()-1; p++)
        {
            sort(Cbest[s][p].begin(), Cbest[s][p].end(), [&](int x, int y){return D[x]<D[y];});
            for (int i=0; i<(int)Cbest[s][p].size(); i++)
            {
                ans_k.push_back(Cbest[s][p][i]);
                ans_x.push_back(X[i]);
                ans_y.push_back(Y[i]);
                ans_s.push_back(SPbest[s][p]);
            }
        }
    }

    // 出力
    cout<<ans_k.size()<<endl;
    for (int i=0; i<(int)ans_k.size(); i++)
        cout<<ans_k[i]+1<<" "<<ans_y[i]<<" "<<ans_x[i]<<" "<<ans_s[i]+1<<"\n";

    system_clock::time_point end = system_clock::now();

    if (set<int>(ans_k.begin(), ans_k.end()).size()!=ans_k.size())
        cerr<<"error"<<endl;

    // 情報出力
    {
        /*
        // 区画数とスコア
        for (int s=0; s<sn; s++)
        {
            int sc = 0;
            for (auto &c: C[s])
                for (int x: c)
                    sc += 25*(D[x]-S[x]+1);
            cerr<<ST[s]->cn<<" "<<sc<<endl;
        }
        */

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

        fprintf(stderr, "%d %4d %5.3f %6d %5.3f %6d\n", d, K, (double)L/(H*W*T), iter, time, score_best);
    }
}
