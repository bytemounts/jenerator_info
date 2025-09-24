using Microsoft.EntityFrameworkCore;

namespace generator_web.Models
{
    public class AppDbContext : DbContext
    {
        public AppDbContext(DbContextOptions<AppDbContext> options)
            : base(options) { }

        public DbSet<generator_data> generator_datas{ get; set; }
        public DbSet<user_data> user_datas { get; set; }
    }
}
