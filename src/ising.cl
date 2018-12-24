kernel void ising(global char* istate, global char* fstate,
   constant float* prob, constant uint size_x)
{
   uint k = get_global_id(0), Kt = get_global_size(0);
   rand_t randstate = init_random((rand_t)k);

   char self_s = istate[k];
   char4 neigh_s = (istate[(k-1)%Kt],
                    istate[(k+1)%Kt],
                    istate[(k-size_x)%Kt],
                    istate[(k+size_x)%Kt]);

   char s_sum = selfs*(neigh_s.x + neigh_s.y + neigh_s.z + neigh_s.w);
   float rand_sample = get_random(&randstate);

   fstate[k] = selfs * (2 * (rand_sample < prob[s_sum*(s_sum>=0)]) - 1);
   
/* This line is equivalent to:
   if(s_sum > 0)
   {
      if(rand_sample < prob[s_sum])
      {
         fstate[k] = -selfs;
      }
      else
      {
         fstate[k] = selfs;
      }
   }
   else
   {
      if(rand_sample < prob[0])
      {
         fstate[k] = -selfs;
      }
      else
      {
         fstate[k] = selfs;
      }
   }
*/
}
