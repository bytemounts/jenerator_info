using Microsoft.EntityFrameworkCore;
using System.ComponentModel.DataAnnotations;

namespace generator_web.Models
{
    public class user_data
    {
        [Key]
        public int userId { get; set; }
        public string command{ get; set; }
        public DateTime DateTime{ get; set; }
    }
}
