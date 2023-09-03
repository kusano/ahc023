#include <iostream>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <functional>
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

struct Input
{
    int T = 0;
    int H = 0;
    int W = 0;
    int y0 = 0;
    vector<vector<int>> h;
    vector<vector<int>> v;
    int K;
    vector<int> S;
    vector<int> D;
};

struct Output
{
    int M = 0;
    vector<int> k;
    vector<int> y;
    vector<int> x;
    vector<int> s;
};

int DX[] = {1, -1, 0, 0};
int DY[] = {0, 0, 1, -1};

bool check(const Input &input, const Output &output)
{
    vector<vector<bool>> F(input.H, vector<bool>(input.W));
    for (int t=0; t<input.T; t++)
    {
        vector<vector<bool>> A(input.H, vector<bool>(input.W));
        function<void(int, int)> f = [&](int x, int y)
        {
            if (A[y][x])
                return;
            A[y][x] = true;
            for (int d=0; d<4; d++)
            {
                int tx = x+DX[d];
                int ty = y+DY[d];
                if (0<=tx && tx<input.W &&
                    0<=ty && ty<input.H &&
                    !F[ty][tx] &&
                    (y==ty || input.h[min(y, ty)][x]==0) &&
                    (x==tx || input.v[y][min(x, tx)]==0))
                    f(tx, ty);
            }
        };
        if (!F[input.y0][0])
            f(0, input.y0);

        for (int i=0; i<output.M; i++)
            if (output.s[i]==t)
            {
                if (!A[output.y[i]][output.x[i]])
                    return false;
                A[output.y[i]][output.x[i]] = false;
                F[output.y[i]][output.x[i]] = true;
            }

        for (int i=0; i<output.M; i++)
            if (input.D[output.k[i]]==t)
                F[output.y[i]][output.x[i]] = false;

        A = vector<vector<bool>>(input.H, vector<bool>(input.W));
        if (!F[input.y0][0])
            f(0, input.y0);

        for (int i=0; i<output.M; i++)
            if (input.D[output.k[i]]==t)
                if (!A[output.y[i]][output.x[i]])
                    return false;
    }
    return true;
}

int calc_score(const Input &input, const Output &output)
{
    int score = 0;
    for (int i=0; i<output.M; i++)
        score += (input.D[output.k[i]]-input.S[output.k[i]]+1)*1000000/(input.H*input.W*input.T);
    return score;
}

int main()
{
    Input input;
    cin>>input.T>>input.H>>input.W>>input.y0;
    input.h = vector<vector<int>>(input.H-1, vector<int>(input.W));
    for (int y=0; y<input.H-1; y++)
    {
        string t;
        cin>>t;
        for (int x=0; x<input.W; x++)
            input.h[y][x] = t[x]-'0';
    }
    input.v = vector<vector<int>>(input.H, vector<int>(input.W-1));
    for (int y=0; y<input.H; y++)
    {
        string t;
        cin>>t;
        for (int x=0; x<input.W-1; x++)
            input.v[y][x] = t[x]-'0';
    }
    cin>>input.K;
    input.S = vector<int>(input.K);
    input.D = vector<int>(input.K);
    for (int k=0; k<input.K; k++)
    {
        cin>>input.S[k]>>input.D[k];
        input.S[k]--;
        input.D[k]--;
    }

    system_clock::time_point start = system_clock::now();

    my_exp_init();

    Output output;
    int score = 0;
    int best_score = score;
    Output best_output = output;

    double temp_inv;
    int iter;
    for (iter=0; ; iter++)
    {
        if (iter%0x10==0)
        {
            system_clock::time_point now = system_clock::now();
            double time = chrono::duration_cast<chrono::nanoseconds>(now-start).count()*1e-9/TIME;
            if (time>1.0)
                break;
            double temp = 100.*(1.0-time);
            temp_inv = 1./temp;
        }

        Output output2 = output;

        switch (xor64()%4)
        {
        case 0:
        {
            if (output2.M==input.K)
                continue;
            int k = 0;
            while (true)
            {
                k = xor64()%input.K;
                bool ok = true;
                for (int kk: output2.k)
                    if (kk==k)
                        ok = false;
                if (ok)
                    break;
            }
            output2.M++;
            output2.k.push_back(k);
            output2.y.push_back(xor64()%input.H);
            output2.x.push_back(xor64()%input.W);
            output2.s.push_back(xor64()%(input.S[k]+1));
            break;
        }
        case 1:
        {
            if (output2.M==0)
                continue;
            int i=xor64()%output2.M;
            output2.M--;
            output2.k.erase(output2.k.begin()+i);
            output2.y.erase(output2.y.begin()+i);
            output2.x.erase(output2.x.begin()+i);
            output2.s.erase(output2.s.begin()+i);
            break;
        }
        case 2:
        {
            if (output2.M==0)
                continue;
            int i=xor64()%output2.M;
            output2.y[i] = xor64()%input.H;
            output2.x[i] = xor64()%input.W;
            break;
        }
        case 3:
        {
            if (output2.M==0)
                continue;
            int i=xor64()%output2.M;
            output2.s[i] = xor64()%(input.S[output2.k[i]]+1);
            break;
        }
        }

        if (!check(input, output2))
            continue;

        int score2 = calc_score(input, output2);

        if (score2>score ||
            //exp((score2-score)*temp_inv)*0x80000000>xor64())
            my_exp((score2-score)*temp_inv)>xor64())
        {
            score = score2;
            output = output2;

            if (score>best_score)
            {
                best_score = score;
                best_output = output;
            }
        }
    }

    cerr<<"Iteration: "<<iter<<endl;
    cerr<<"Score: "<<best_score<<endl;

    cout<<output.M<<endl;
    for (int i=0; i<output.M; i++)
        cout<<output.k[i]+1<<" "<<output.y[i]<<" "<<output.x[i]<<" "<<output.s[i]+1<<endl;
}
